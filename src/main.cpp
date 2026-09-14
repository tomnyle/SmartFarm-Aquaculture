#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "../include/app_config.h"
#include "../include/pins.h"
#include "../include/system_state.h"
#include "../include/device_config.h"
#include "../include/automation/aquaculture_rules.h"

// Global instances
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
AquacultureSystemState system_state;
DeviceConfig device_config;
AquacultureRuleEngine rule_engine;

uint32_t last_rule_run = 0;
uint32_t last_mqtt_update = 0;

// Forward declarations
void setupWiFi();
void setupMQTT();
void initializeSensors();
void initializeOutputs();
void handleRuleEngine();
void publishState();
void mqttCallback(char* topic, byte* payload, unsigned int length);

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n=== SmartFarm Aquaculture Controller V" FW_VERSION " ===");
    Serial.println("Initializing...");
    
    // Initialize state
    StateManager::initializeState(system_state);
    
    // Load device config
    ConfigManager::loadDefaultConfig(device_config);
    
    // Initialize sensors
    initializeSensors();
    
    // Initialize outputs
    initializeOutputs();
    
    // Load profile
    // TODO: Load active profile
    
    // Setup WiFi
    setupWiFi();
    
    // Setup MQTT
    setupMQTT();
    
    Serial.println("Initialization complete!");
    StateManager::setSystemStatus(system_state, STATUS_RUNNING);
}

void loop() {
    // Maintain WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        setupWiFi();
    }
    
    // Maintain MQTT connection
    if (!mqtt_client.connected()) {
        setupMQTT();
    } else {
        mqtt_client.loop();
    }
    
    uint32_t now = millis();
    
    // Run rule engine at regular interval
    if (now - last_rule_run >= RULE_ENGINE_INTERVAL) {
        handleRuleEngine();
        last_rule_run = now;
    }
    
    // Publish state at regular interval
    if (now - last_mqtt_update >= device_config.mqtt_update_interval) {
        publishState();
        last_mqtt_update = now;
    }
    
    delay(10);
}

void setupWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }
    
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi connected! IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println("WiFi connection failed!");
    }
}

void setupMQTT() {
    mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt_client.setCallback(mqttCallback);
    
    if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
        Serial.println("MQTT connected!");
        mqtt_client.subscribe(MQTT_TOPIC_CONTROL);
        mqtt_client.subscribe(MQTT_TOPIC_CONFIG);
    } else {
        Serial.print("MQTT connection failed, rc=");
        Serial.println(mqtt_client.state());
    }
}

void initializeSensors() {
    Serial.println("Initializing sensors...");
    // TODO: Initialize each sensor based on config
}

void initializeOutputs() {
    Serial.println("Initializing outputs...");
    // TODO: Initialize relay pins
}

void handleRuleEngine() {
    // Read sensor values
    float temp = system_state.sensors.water_temperature;
    float ph = system_state.sensors.water_ph;
    float dissolved_oxygen = system_state.sensors.dissolved_oxygen;
    float water_level = system_state.sensors.water_level;
    
    // Evaluate rules
    RuleDecision decision = rule_engine.evaluate(temp, ph, dissolved_oxygen, water_level);
    
    // Apply outputs based on decision
    // TODO: Apply outputs
}

void publishState() {
    JsonDocument doc;
    doc = StateManager::stateToJson(system_state);
    
    String payload;
    serializeJson(doc, payload);
    
    mqtt_client.publish(MQTT_TOPIC_STATE, payload.c_str());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("MQTT message received on topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    Serial.println(message);
    
    // TODO: Handle incoming MQTT messages
}
