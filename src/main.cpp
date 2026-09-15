#include <Arduino.h>
#include <cstring>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "../include/config.h"
#include "../include/types.h"
#include "config/profiles.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "relays/relay_manager.h"
#include "rules/rule_engine.h"
#include "rules/profiles.h"
#include "sensors/temperature/ds18b20_driver.h"
#include "sensors/ph/ph_sensor_driver.h"
#include "sensors/dissolved_oxygen/do_sensor_driver.h"
#include "sensors/water_level/water_level_driver.h"
#include "sensors/ec_tds/ec_tds_driver.h"
#include "sensors/orp/orp_driver.h"
#include "sensors/turbidity/turbidity_driver.h"
#include "sensors/co2/co2_driver.h"
#include "sensors/gas_sensor/gas_sensor_driver.h"
#include "sensors/sensors.h"
#include "utils/logger.h"

namespace {
// Core services / Dịch vụ lõi dùng xuyên suốt vòng đời firmware.
WiFiManager wifi_manager;
MqttManager mqtt_manager;
RelayManager relay_manager;
RuleEngine rule_engine;
Adafruit_ADS1115 ads_primary;
Adafruit_ADS1115 ads_secondary;
bool ads_primary_ready = false;
bool ads_secondary_ready = false;

// Concrete drivers / Driver cụ thể cho từng cảm biến theo phase.
DS18B20Driver temperature_driver(TEMPERATURE_SENSOR_CONFIG);
PHSensorDriver ph_driver(&ads_primary, PH_SENSOR_CONFIG);
DOSensorDriver do_driver(&ads_primary, DO_SENSOR_CONFIG);
WaterLevelDriver water_level_driver(WATER_LEVEL_SENSOR_CONFIG);
ECTDSDriver ec_tds_driver(EC_TDS_SENSOR_CONFIG);
ORPDriver orp_driver(ORP_SENSOR_CONFIG);
TurbidityDriver turbidity_driver(TURBIDITY_SENSOR_CONFIG);
CO2Driver co2_driver(CO2_SENSOR_CONFIG);
GasSensorDriver gas_sensor_driver(GAS_SENSOR_CONFIG);
SensorSnapshot sensor_snapshot = {
    {0, 0.0F, ENABLE_TEMPERATURE ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_PH ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_DO ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_WATER_LEVEL ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_EC_TDS ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_ORP ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_TURBIDITY ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_CO2 ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0},
    {0, 0.0F, ENABLE_GAS_SENSOR ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0}
};
struct SensorBinding {
    SensorDriver* driver;
    SensorReading* slot;
};

SensorBinding sensor_bindings[] = {
    {&temperature_driver, &sensor_snapshot.temperature},
    {&ph_driver, &sensor_snapshot.ph},
    {&do_driver, &sensor_snapshot.dissolved_oxygen},
    {&water_level_driver, &sensor_snapshot.water_level},
    {&ec_tds_driver, &sensor_snapshot.ec},
    {&orp_driver, &sensor_snapshot.orp},
    {&turbidity_driver, &sensor_snapshot.turbidity},
    {&co2_driver, &sensor_snapshot.co2},
    {&gas_sensor_driver, &sensor_snapshot.gas_sensor}
};
SystemState system_state = SYSTEM_INIT;
const SpeciesProfile* active_profile = nullptr;
char current_mode[16] = "AUTO";
uint32_t last_sensor_read = 0;
uint32_t last_rule_run = 0;
uint32_t last_publish = 0;

SensorReading buildErrorReading() { return {millis(), 0.0F, SENSOR_STATUS_ERROR, 1}; }

// MQTT manual override / Điều khiển relay thủ công từ MQTT.
void handleRelayCommand(const char* relay_name, RelayState state) {
    if (relay_manager.setRelayState(relay_name, state)) {
        strcpy(current_mode, "MANUAL");
    }
}

// Mode validation / Chỉ nhận các mode hợp lệ để tránh trạng thái rác.
void handleModeCommand(const char* mode) {
    if (strcmp(mode, "AUTO") != 0 &&
        strcmp(mode, "MANUAL") != 0 &&
        strcmp(mode, "SCHEDULE") != 0 &&
        strcmp(mode, "SAFE") != 0) {
        return;
    }

    strncpy(current_mode, mode, sizeof(current_mode) - 1);
    current_mode[sizeof(current_mode) - 1] = '\0';
}

// Shared buses / Khởi tạo bus chung chỉ khi thật sự cần dùng.
void initializeBuses() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    const bool primary_needed = ENABLE_PH || ENABLE_DO;
    const bool secondary_needed = ENABLE_EC_TDS || ENABLE_ORP || ENABLE_TURBIDITY || ENABLE_CO2 || ENABLE_GAS_SENSOR;

    if (primary_needed) {
        ads_primary_ready = ads_primary.begin(ADS1115_PRIMARY_ADDRESS);
    }
    if (secondary_needed) {
        ads_secondary_ready = ads_secondary.begin(ADS1115_SECONDARY_ADDRESS);
    }
}

// Runtime skip for disabled sensors / Bỏ qua cảm biến đã tắt trong config.
void initializeSensors() {
    for (auto& binding : sensor_bindings) {
        if (binding.driver->isEnabled()) {
            if (!binding.driver->begin()) {
                *binding.slot = buildErrorReading();
            }
        }
    }
}

bool isBusReady(uint8_t bus_index) {
    if (bus_index == 1) {
        return ads_primary_ready;
    }
    if (bus_index == 2) {
        return ads_secondary_ready;
    }
    return true;
}

// Generic polling / Đọc toàn bộ driver qua interface trừu tượng.
void readSensors() {
    for (auto& binding : sensor_bindings) {
        if (!isBusReady(binding.driver->getBusIndex())) {
            *binding.slot = buildErrorReading();
            continue;
        }
        *binding.slot = binding.driver->read();
    }
}

// Auto rules / SAFE mode ép đầu ra an toàn, AUTO mode chạy rule theo profile.
void applyRulesIfNeeded() {
    if (strcmp(current_mode, "SAFE") == 0) {
        relay_manager.setRelayState("aerator", RELAY_ON);
        relay_manager.setRelayState("pump", RELAY_OFF);
        relay_manager.setRelayState("circulation", RELAY_OFF);
        relay_manager.setRelayState("feeder", RELAY_OFF);
        relay_manager.setRelayState("valve", RELAY_OFF);
        relay_manager.setRelayState("light", RELAY_OFF);
        relay_manager.setRelayState("spare_1", RELAY_OFF);
        relay_manager.setRelayState("spare_2", RELAY_OFF);
        system_state = SYSTEM_SAFE;
        return;
    }

    if (strcmp(current_mode, "AUTO") != 0) {
        return;
    }

    const RuleEvaluation evaluation = rule_engine.evaluate(sensor_snapshot, *active_profile);
    relay_manager.setRelayState("aerator", evaluation.aerator);
    relay_manager.setRelayState("pump", evaluation.pump);
    relay_manager.setRelayState("circulation", evaluation.circulation);
    relay_manager.setRelayState("feeder", evaluation.feeder);
    relay_manager.setRelayState("valve", evaluation.valve);
    relay_manager.setRelayState("light", evaluation.light);
    relay_manager.setRelayState("spare_1", evaluation.spare_1);
    relay_manager.setRelayState("spare_2", evaluation.spare_2);
    system_state = evaluation.system_state;
}
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Logger::info("Starting SmartFarm Aquaculture firmware / Khoi dong firmware SmartFarm Aquaculture");
    active_profile = getSpeciesProfile(DEFAULT_SPECIES_PROFILE);
    if (active_profile == nullptr) {
        active_profile = getSpeciesProfile("tilapia");
    }

    relay_manager.begin();
    initializeBuses();
    initializeSensors();

    wifi_manager.connect();
    mqtt_manager.begin(APP_DEVICE_ID);
    mqtt_manager.setRelayCommandHandler(handleRelayCommand);
    mqtt_manager.setModeCommandHandler(handleModeCommand);

    system_state = SYSTEM_READY;
}

void loop() {
    bool wifi_connected = wifi_manager.isConnected();
    if (!wifi_connected) {
        wifi_manager.connect();
        wifi_connected = wifi_manager.isConnected();
    }

    if (wifi_connected && mqtt_manager.ensureConnected()) {
        mqtt_manager.loop();
    }

    const uint32_t now = millis();

    if (now - last_sensor_read >= SENSOR_READ_INTERVAL_MS) {
        readSensors();
        last_sensor_read = now;
    }

    if (now - last_rule_run >= RULE_ENGINE_INTERVAL_MS) {
        applyRulesIfNeeded();
        last_rule_run = now;
    }

    if (now - last_publish >= MQTT_PUBLISH_INTERVAL_MS && mqtt_manager.isConnected()) {
        mqtt_manager.publishDiscovery(relay_manager, sensor_snapshot);
        mqtt_manager.publishSensors(sensor_snapshot);
        mqtt_manager.publishRelayStates(relay_manager);
        mqtt_manager.publishSystemState(system_state, active_profile != nullptr ? active_profile->id : "tilapia", current_mode);
        if (system_state == SYSTEM_ERROR || system_state == SYSTEM_SAFE) {
            mqtt_manager.publishError(system_state == SYSTEM_SAFE ? "SAFE_MODE_ACTIVE" : "SENSOR_READ_ERROR");
        }
        last_publish = now;
    }

    if (system_state == SYSTEM_READY) {
        system_state = SYSTEM_RUNNING;
    }

    delay(10);
}
