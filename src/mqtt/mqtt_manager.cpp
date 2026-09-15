#include "mqtt_manager.h"
#include <ArduinoJson.h>
#include "topics.h"
#include "../config/app_config.h"
#include "../relays/relay_names.h"
#include "../utils/helpers.h"
#include "../../include/version.h"

MQTTManager* MQTTManager::instance_ = nullptr;

namespace {
const char* modeToString(OperationMode mode) {
  switch (mode) {
    case OperationMode::AUTO: return "AUTO";
    case OperationMode::MANUAL: return "MANUAL";
    case OperationMode::SCHEDULE: return "SCHEDULE";
    case OperationMode::SAFE: return "SAFE";
  }
  return "AUTO";
}

OperationMode stringToMode(const String& value) {
  if (value.equalsIgnoreCase("MANUAL")) return OperationMode::MANUAL;
  if (value.equalsIgnoreCase("SCHEDULE")) return OperationMode::SCHEDULE;
  if (value.equalsIgnoreCase("SAFE")) return OperationMode::SAFE;
  return OperationMode::AUTO;
}
}

MQTTManager::MQTTManager() : client_(wifiClient_) { instance_ = this; }

void MQTTManager::begin() {
  client_.setServer(app_config::MQTT_HOST, app_config::MQTT_PORT);
  client_.setCallback(MQTTManager::callback);
  client_.setBufferSize(2048);
}

bool MQTTManager::connected() const { return client_.connected(); }

void MQTTManager::loop() {
  if (!client_.connected()) {
    client_.connect(app_config::DEVICE_ID, app_config::MQTT_USERNAME, app_config::MQTT_PASSWORD,
                    mqtt_topics::availability().c_str(), 1, true, "offline");
    if (client_.connected()) {
      client_.publish(mqtt_topics::availability().c_str(), "online", true);
      subscribeTopics();
    }
  }
  client_.loop();
}

void MQTTManager::subscribeTopics() {
  client_.subscribe(mqtt_topics::modeSet().c_str());
  client_.subscribe(mqtt_topics::profileSet().c_str());
  client_.subscribe(mqtt_topics::scheduleSet().c_str());
  const char* relays[] = {relay_names::AERATOR, relay_names::PUMP, relay_names::CIRCULATION, relay_names::FEEDER,
                          relay_names::VALVE, relay_names::LIGHT, relay_names::SPARE1, relay_names::SPARE2};
  for (const char* relay : relays) {
    client_.subscribe(mqtt_topics::relaySet(relay).c_str());
  }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
  if (!instance_) return;
  String message;
  for (unsigned int i = 0; i < length; ++i) message += static_cast<char>(payload[i]);
  instance_->handleMessage(topic, message);
}

void MQTTManager::handleMessage(const char* topic, const String& payload) {
  if (String(topic) == mqtt_topics::modeSet()) {
    pendingMode_ = stringToMode(payload);
    hasModeCommand_ = true;
    return;
  }
  if (String(topic) == mqtt_topics::profileSet()) {
    pendingProfile_ = payload;
    hasProfileCommand_ = true;
    return;
  }
  if (String(topic) == mqtt_topics::scheduleSet()) {
    pendingSchedule_ = payload;
    hasScheduleCommand_ = true;
    return;
  }
  const char* relays[] = {relay_names::AERATOR, relay_names::PUMP, relay_names::CIRCULATION, relay_names::FEEDER,
                          relay_names::VALVE, relay_names::LIGHT, relay_names::SPARE1, relay_names::SPARE2};
  for (const char* relay : relays) {
    if (String(topic) == mqtt_topics::relaySet(relay)) {
      pendingRelayName_ = relay;
      pendingRelayState_ = payload.equalsIgnoreCase("ON");
      hasRelayCommand_ = true;
      return;
    }
  }
}

bool MQTTManager::consumeRelayCommand(String& relayName, bool& relayState) {
  if (!hasRelayCommand_) return false;
  relayName = pendingRelayName_;
  relayState = pendingRelayState_;
  hasRelayCommand_ = false;
  return true;
}

bool MQTTManager::consumeModeCommand(OperationMode& mode) {
  if (!hasModeCommand_) return false;
  mode = pendingMode_;
  hasModeCommand_ = false;
  return true;
}

bool MQTTManager::consumeProfileCommand(String& profile) {
  if (!hasProfileCommand_) return false;
  profile = pendingProfile_;
  hasProfileCommand_ = false;
  return true;
}

bool MQTTManager::consumeScheduleCommand(String& payload) {
  if (!hasScheduleCommand_) return false;
  payload = pendingSchedule_;
  hasScheduleCommand_ = false;
  return true;
}

void MQTTManager::publishDiscovery(const SystemState& state) const {
  if (client_.connected()) discovery_.publishAll(client_, state);
}

void MQTTManager::publishState(const SystemState& systemState, const SensorManager& sensors, const RelayManager& relays) const {
  if (!client_.connected()) return;

  const SensorReading sensorValues[] = {sensors.getTemperature(), sensors.getPH(), sensors.getDO(), sensors.getWaterLevel(),
                                        sensors.getEC(), sensors.getORP()};
  for (const auto& reading : sensorValues) {
    if (reading.available) {
      client_.publish(mqtt_topics::sensor(reading.key).c_str(), String(reading.value, 2).c_str(), true);
    }
  }

  client_.publish(mqtt_topics::mode().c_str(), modeToString(systemState.mode), true);
  client_.publish(mqtt_topics::profile().c_str(), systemState.activeProfile.c_str(), true);
  client_.publish(mqtt_topics::systemState().c_str(), systemState.status.c_str(), true);
  client_.publish(mqtt_topics::condition("temperature").c_str(), helpers::conditionToString(systemState.conditions.temperature), true);
  client_.publish(mqtt_topics::condition("ph").c_str(), helpers::conditionToString(systemState.conditions.ph), true);
  client_.publish(mqtt_topics::condition("do").c_str(), helpers::conditionToString(systemState.conditions.dissolvedOxygen), true);
  client_.publish(mqtt_topics::condition("water_level").c_str(), helpers::conditionToString(systemState.conditions.waterLevel), true);
  client_.publish(mqtt_topics::error().c_str(), systemState.error.c_str(), true);

  for (uint8_t i = 0; i < 8; ++i) {
    const RelaySnapshot& snapshot = relays.snapshots()[i];
    client_.publish(mqtt_topics::relay(snapshot.name).c_str(), snapshot.state ? "ON" : "OFF", true);
  }

  StaticJsonDocument<1024> doc;
  doc["device_id"] = app_config::DEVICE_ID;
  doc["firmware"] = SMARTFARM_FIRMWARE_VERSION;
  doc["mode"] = modeToString(systemState.mode);
  doc["profile"] = systemState.activeProfile;
  doc["system_state"] = systemState.status;
  doc["error"] = systemState.error;
  const SensorReading temperature = sensors.getTemperature();
  const SensorReading ph = sensors.getPH();
  const SensorReading dissolvedOxygen = sensors.getDO();
  const SensorReading waterLevel = sensors.getWaterLevel();
  doc["sensors"]["temperature"]["available"] = temperature.available;
  if (temperature.available) doc["sensors"]["temperature"]["value"] = temperature.value;
  doc["sensors"]["ph"]["available"] = ph.available;
  if (ph.available) doc["sensors"]["ph"]["value"] = ph.value;
  doc["sensors"]["do"]["available"] = dissolvedOxygen.available;
  if (dissolvedOxygen.available) doc["sensors"]["do"]["value"] = dissolvedOxygen.value;
  doc["sensors"]["water_level"]["available"] = waterLevel.available;
  if (waterLevel.available) doc["sensors"]["water_level"]["value"] = waterLevel.value;
  doc["conditions"]["temperature"] = helpers::conditionToString(systemState.conditions.temperature);
  doc["conditions"]["ph"] = helpers::conditionToString(systemState.conditions.ph);
  doc["conditions"]["do"] = helpers::conditionToString(systemState.conditions.dissolvedOxygen);
  doc["conditions"]["water_level"] = helpers::conditionToString(systemState.conditions.waterLevel);
  for (uint8_t i = 0; i < 8; ++i) {
    doc["relays"][relays.snapshots()[i].name] = relays.snapshots()[i].state ? "ON" : "OFF";
  }
  String payload;
  serializeJson(doc, payload);
  client_.publish(mqtt_topics::state().c_str(), payload.c_str(), true);
}
