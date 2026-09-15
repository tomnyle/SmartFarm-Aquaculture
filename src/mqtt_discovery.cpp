#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "../include/app_config.h"

// Home Assistant MQTT Discovery
// Automatically creates entities in Home Assistant

extern PubSubClient mqtt_client;

void publishTemperatureSensorDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Temperature";
    doc["unique_id"] = "aquaculture_temperature";
    doc["state_topic"] = MQTT_TOPIC_WATER_TEMP;
    doc["unit_of_measurement"] = "°C";
    doc["device_class"] = "temperature";
    doc["icon"] = "mdi:thermometer";

    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    doc["device"]["model"] = "ESP32 Aquaculture V0.1";
    doc["device"]["manufacturer"] = "SmartFarm";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/sensor/aquaculture_temperature/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Temperature sensor registered");
}

void publishPHSensorDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture pH";
    doc["unique_id"] = "aquaculture_ph";
    doc["state_topic"] = MQTT_TOPIC_PH;
    doc["unit_of_measurement"] = "pH";
    doc["icon"] = "mdi:test-tube";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/sensor/aquaculture_ph/config", payload.c_str(), true);
    Serial.println("[HA Discovery] pH sensor registered");
}

void publishDOSensorDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Dissolved Oxygen";
    doc["unique_id"] = "aquaculture_do";
    doc["state_topic"] = MQTT_TOPIC_DO;
    doc["unit_of_measurement"] = "mg/L";
    doc["icon"] = "mdi:water";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/sensor/aquaculture_do/config", payload.c_str(), true);
    Serial.println("[HA Discovery] DO sensor registered");
}

void publishWaterLevelSensorDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Water Level";
    doc["unique_id"] = "aquaculture_level";
    doc["state_topic"] = MQTT_TOPIC_TURBIDITY;
    doc["unit_of_measurement"] = "%";
    doc["icon"] = "mdi:water-percent";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/sensor/aquaculture_level/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Water level sensor registered");
}

void publishAeratorSwitchDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Aerator";
    doc["unique_id"] = "aquaculture_aerator";
    doc["state_topic"] = MQTT_TOPIC_AERATOR;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_AERATOR;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:air-purifier";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/switch/aquaculture_aerator/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Aerator switch registered");
}

void publishWaterPumpSwitchDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Water Pump";
    doc["unique_id"] = "aquaculture_water_pump";
    doc["state_topic"] = MQTT_TOPIC_PUMP;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_PUMP;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:pump";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/switch/aquaculture_water_pump/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Water pump switch registered");
}

void publishCirculationSwitchDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Circulation";
    doc["unique_id"] = "aquaculture_circulation";
    doc["state_topic"] = MQTT_TOPIC_CIRCULATION;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_CIRCULATION;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:water-pump";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/switch/aquaculture_circulation/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Circulation switch registered");
}

void publishFeederSwitchDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Feeder";
    doc["unique_id"] = "aquaculture_feeder";
    doc["state_topic"] = MQTT_TOPIC_FEEDER;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_FEEDER;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:fish-food";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/switch/aquaculture_feeder/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Feeder switch registered");
}

void publishSpeciesSelectDiscovery() {
    StaticJsonDocument<768> doc;
    doc["name"] = "Aquaculture Species";
    doc["unique_id"] = "aquaculture_species";
    doc["state_topic"] = MQTT_TOPIC_SPECIES_STATE;
    doc["command_topic"] = MQTT_TOPIC_CONFIG_SPECIES;
    doc["icon"] = "mdi:fish";
    doc["options"][0] = "Koi";
    doc["options"][1] = "Cá Trắm";
    doc["options"][2] = "Cá Chép";
    doc["options"][3] = "Cá Tra";
    doc["options"][4] = "Cá Lóc";
    doc["options"][5] = "Tôm Thẻ";
    doc["options"][6] = "Tôm Sú";
    doc["options"][7] = "Tilapia";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/select/aquaculture_species/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Species selector registered");
}

void publishModeSelectorDiscovery() {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Mode";
    doc["unique_id"] = "aquaculture_mode";
    doc["state_topic"] = MQTT_TOPIC_MODE_STATE;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_MODE;
    doc["icon"] = "mdi:cog";
    doc["options"][0] = "AUTO";
    doc["options"][1] = "MANUAL";
    doc["options"][2] = "SCHEDULE";
    doc["options"][3] = "SAFE";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);

    mqtt_client.publish("homeassistant/select/aquaculture_mode/config", payload.c_str(), true);
    Serial.println("[HA Discovery] Mode selector registered");
}

void publishAllDiscoveries() {
    Serial.println("\n=== Publishing Home Assistant Discoveries ===");

    publishTemperatureSensorDiscovery();
    publishPHSensorDiscovery();
    publishDOSensorDiscovery();
    publishWaterLevelSensorDiscovery();

    publishAeratorSwitchDiscovery();
    publishWaterPumpSwitchDiscovery();
    publishCirculationSwitchDiscovery();
    publishFeederSwitchDiscovery();

    publishSpeciesSelectDiscovery();
    publishModeSelectorDiscovery();

    Serial.println("=== All discoveries published ===");
}