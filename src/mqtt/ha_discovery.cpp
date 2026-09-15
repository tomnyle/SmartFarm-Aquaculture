#include "ha_discovery.h"
#include <ArduinoJson.h>
#include "topics.h"
#include "../config/app_config.h"
#include "../relays/relay_names.h"
#include "../rules/profiles.h"
#include "../../include/version.h"

namespace {
void addDevice(JsonDocument& doc) {
  doc["device"]["identifiers"][0] = app_config::DEVICE_ID;
  doc["device"]["name"] = app_config::DEVICE_NAME;
  doc["device"]["manufacturer"] = "SmartFarm";
  doc["device"]["model"] = "ESP32 Aquaculture";
  doc["device"]["sw_version"] = SMARTFARM_FIRMWARE_VERSION;
}

String discoveryTopic(const char* component, const char* objectId) {
  return String(app_config::HA_DISCOVERY_PREFIX) + "/" + component + "/" + app_config::DEVICE_ID + "/" + objectId + "/config";
}
}

void HADiscovery::publishSensor(PubSubClient& client, const char* objectId, const char* name,
                                const String& stateTopic, const char* unit, const char* icon) const {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = String(app_config::DEVICE_ID) + "_" + objectId;
  doc["state_topic"] = stateTopic;
  doc["availability_topic"] = mqtt_topics::availability();
  if (unit && unit[0] != '\0') doc["unit_of_measurement"] = unit;
  if (icon && icon[0] != '\0') doc["icon"] = icon;
  addDevice(doc);
  String payload;
  serializeJson(doc, payload);
  client.publish(discoveryTopic("sensor", objectId).c_str(), payload.c_str(), true);
}

void HADiscovery::publishSwitch(PubSubClient& client, const char* objectId, const char* name,
                                const char* relayName, const char* icon) const {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = String(app_config::DEVICE_ID) + "_" + objectId;
  doc["state_topic"] = mqtt_topics::relay(relayName);
  doc["command_topic"] = mqtt_topics::relaySet(relayName);
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["availability_topic"] = mqtt_topics::availability();
  doc["icon"] = icon;
  addDevice(doc);
  String payload;
  serializeJson(doc, payload);
  client.publish(discoveryTopic("switch", objectId).c_str(), payload.c_str(), true);
}

void HADiscovery::publishSelect(PubSubClient& client, const char* objectId, const char* name,
                                const String& stateTopic, const String& commandTopic,
                                const char* const* options, uint8_t optionCount, const char* icon) const {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = String(app_config::DEVICE_ID) + "_" + objectId;
  doc["state_topic"] = stateTopic;
  doc["command_topic"] = commandTopic;
  doc["availability_topic"] = mqtt_topics::availability();
  doc["icon"] = icon;
  for (uint8_t i = 0; i < optionCount; ++i) {
    doc["options"][i] = options[i];
  }
  addDevice(doc);
  String payload;
  serializeJson(doc, payload);
  client.publish(discoveryTopic("select", objectId).c_str(), payload.c_str(), true);
}

void HADiscovery::publishStatusSensor(PubSubClient& client, const char* objectId, const char* name,
                                      const String& stateTopic, const char* icon) const {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = String(app_config::DEVICE_ID) + "_" + objectId;
  doc["state_topic"] = stateTopic;
  doc["availability_topic"] = mqtt_topics::availability();
  doc["icon"] = icon;
  addDevice(doc);
  String payload;
  serializeJson(doc, payload);
  client.publish(discoveryTopic("sensor", objectId).c_str(), payload.c_str(), true);
}

void HADiscovery::publishAll(PubSubClient& client, const SystemState&) const {
  publishSensor(client, "temperature", "Aquaculture Temperature", mqtt_topics::sensor("temperature"), "°C", "mdi:thermometer");
  publishSensor(client, "ph", "Aquaculture pH", mqtt_topics::sensor("ph"), "pH", "mdi:test-tube");
  publishSensor(client, "do", "Aquaculture DO", mqtt_topics::sensor("do"), "mg/L", "mdi:water");
  publishSensor(client, "water_level", "Aquaculture Water Level", mqtt_topics::sensor("water_level"), "%", "mdi:water-percent");
  publishStatusSensor(client, "condition_temperature", "Temperature Condition", mqtt_topics::condition("temperature"), "mdi:thermometer-alert");
  publishStatusSensor(client, "condition_ph", "pH Condition", mqtt_topics::condition("ph"), "mdi:test-tube");
  publishStatusSensor(client, "condition_do", "DO Condition", mqtt_topics::condition("do"), "mdi:water-alert");
  publishStatusSensor(client, "condition_water_level", "Water Level Condition", mqtt_topics::condition("water_level"), "mdi:waves-arrow-down");
  publishStatusSensor(client, "system_state", "System State", mqtt_topics::systemState(), "mdi:information");
  publishStatusSensor(client, "system_error", "System Error", mqtt_topics::error(), "mdi:alert-circle");

  publishSwitch(client, "aerator", "Aerator", relay_names::AERATOR, "mdi:air-purifier");
  publishSwitch(client, "pump", "Pump", relay_names::PUMP, "mdi:pump");
  publishSwitch(client, "circulation", "Circulation", relay_names::CIRCULATION, "mdi:water-sync");
  publishSwitch(client, "feeder", "Feeder", relay_names::FEEDER, "mdi:fish-food");
  publishSwitch(client, "valve", "Valve", relay_names::VALVE, "mdi:valve");
  publishSwitch(client, "light", "Light", relay_names::LIGHT, "mdi:lightbulb");
  publishSwitch(client, "spare1", "Spare 1", relay_names::SPARE1, "mdi:toggle-switch");
  publishSwitch(client, "spare2", "Spare 2", relay_names::SPARE2, "mdi:toggle-switch");

  static const char* const modes[] = {"AUTO", "MANUAL", "SCHEDULE", "SAFE"};
  const SpeciesProfile* profiles = rules_profiles::all();
  const uint8_t profileCount = rules_profiles::count();
  const char* profileNames[8] = {};
  for (uint8_t i = 0; i < profileCount && i < 8; ++i) {
    profileNames[i] = profiles[i].name;
  }
  publishSelect(client, "mode", "Operation Mode", mqtt_topics::mode(), mqtt_topics::modeSet(), modes, 4, "mdi:cog");
  publishSelect(client, "profile", "Species Profile", mqtt_topics::profile(), mqtt_topics::profileSet(), profileNames, profileCount, "mdi:fish");
}
