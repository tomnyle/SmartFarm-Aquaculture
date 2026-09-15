#include <Arduino.h>
#include "../include/version.h"
#include "../include/constants.h"
#include "config/app_config.h"
#include "config/profiles.h"
#include "modes/auto_mode.h"
#include "modes/manual_mode.h"
#include "modes/safe_mode.h"
#include "modes/schedule_mode.h"
#include "mqtt/mqtt_manager.h"
#include "relays/relay_manager.h"
#include "rules/profiles.h"
#include "rules/rule_engine.h"
#include "sensors/sensor_manager.h"
#include "system/error_handler.h"
#include "system/system_state.h"
#include "wifi/wifi_manager.h"

namespace {
WiFiManager wifiManager;
MQTTManager mqttManager;
SensorManager sensorManager;
RelayManager relayManager;
RuleEngine ruleEngine;
AutoMode autoMode;
ManualMode manualMode;
ScheduleMode scheduleMode;
SafeMode safeMode;
ErrorHandler errorHandler;
SystemState systemState;
bool discoveryPublished = false;

void handleIncomingCommands() {
  OperationMode requestedMode;
  if (mqttManager.consumeModeCommand(requestedMode)) {
    systemState.selectedMode = requestedMode;
    systemState.mode = requestedMode;
    systemState.status = requestedMode == OperationMode::SAFE ? "SAFE" : "RUNNING";
  }

  String requestedProfile;
  if (mqttManager.consumeProfileCommand(requestedProfile)) {
    const SpeciesProfile* profile = rules_profiles::findByName(requestedProfile.c_str());
    if (rules_profiles::isSupported(requestedProfile.c_str())) {
      systemState.activeProfile = profile->name;
    }
  }

  String schedulePayload;
  if (mqttManager.consumeScheduleCommand(schedulePayload)) {
    scheduleMode.updateFromJson(schedulePayload, systemState.schedule);
  }

  String relayName;
  bool relayState = false;
  if (mqttManager.consumeRelayCommand(relayName, relayState) && systemState.mode == OperationMode::MANUAL) {
    manualMode.applyCommand(relayManager, relayName.c_str(), relayState);
  }
}

void evaluateAutomation() {
  const SpeciesProfile* profile = rules_profiles::findByName(systemState.activeProfile.c_str());
  const RuleEvaluation evaluation = ruleEngine.evaluate(*profile,
                                                        sensorManager.getTemperature(),
                                                        sensorManager.getPH(),
                                                        sensorManager.getDO(),
                                                        sensorManager.getWaterLevel());
  systemState.conditions = evaluation.conditions;

  if (evaluation.safeModeRequired) {
    systemState.mode = OperationMode::SAFE;
    errorHandler.setError(systemState, evaluation.reason);
    systemState.status = "SAFE";
  } else {
    errorHandler.clear(systemState);
    if (systemState.mode == OperationMode::SAFE && systemState.selectedMode != OperationMode::SAFE) {
      systemState.mode = systemState.selectedMode;
    }
    systemState.status = "RUNNING";
  }

  if (systemState.mode == OperationMode::AUTO) {
    autoMode.apply(evaluation, relayManager);
  } else if (systemState.mode == OperationMode::SAFE) {
    safeMode.apply(relayManager);
  }
}
}

void setup() {
  Serial.begin(115200);
  initializeSystemState(systemState, app_config::DEFAULT_PROFILE);
  scheduleMode.configureDefaults(systemState.schedule);
  relayManager.begin();
  sensorManager.begin();
  wifiManager.begin();
  wifiManager.ensureConnected();
  mqttManager.begin();
  mqttManager.loop();
  if (mqttManager.connected()) {
    mqttManager.publishDiscovery(systemState);
    discoveryPublished = true;
  }
  systemState.status = "READY";
}

void loop() {
  wifiManager.ensureConnected();
  mqttManager.loop();
  if (mqttManager.connected() && !discoveryPublished) {
    mqttManager.publishDiscovery(systemState);
    discoveryPublished = true;
  } else if (!mqttManager.connected()) {
    discoveryPublished = false;
  }
  handleIncomingCommands();

  const uint32_t now = millis();
  if (now - systemState.lastSensorPoll >= constants::SENSOR_READ_INTERVAL_MS) {
    sensorManager.poll();
    systemState.lastSensorPoll = now;
  }

  if (now - systemState.lastRuleEvaluation >= constants::RULE_EVALUATION_INTERVAL_MS) {
    evaluateAutomation();
    if (systemState.mode == OperationMode::SCHEDULE) {
      scheduleMode.tick(systemState.schedule, relayManager);
      systemState.status = "RUNNING";
    }
    systemState.lastRuleEvaluation = now;
  }

  if (now - systemState.lastMqttPublish >= constants::MQTT_PUBLISH_INTERVAL_MS) {
    mqttManager.publishState(systemState, sensorManager, relayManager);
    systemState.lastMqttPublish = now;
  }

  delay(50);
}
