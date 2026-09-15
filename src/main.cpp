#include <Arduino.h>
#include <cstring>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "../include/config.h"
#include "../include/types.h"
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
SensorDriver* sensor_drivers[] = {
    &temperature_driver,
    &ph_driver,
    &do_driver,
    &water_level_driver,
    &ec_tds_driver,
    &orp_driver,
    &turbidity_driver,
    &co2_driver,
    &gas_sensor_driver
};
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
SystemState system_state = SYSTEM_INIT;
const SpeciesProfile* active_profile = nullptr;
char current_mode[16] = "AUTO";
uint32_t last_sensor_read = 0;
uint32_t last_rule_run = 0;
uint32_t last_publish = 0;

// MQTT manual override / Điều khiển relay thủ công từ MQTT.
void handleRelayCommand(const char* relay_name, RelayState state) {
    strcpy(current_mode, "MANUAL");
    relay_manager.setRelayState(relay_name, state);
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
    bool primary_needed = ENABLE_PH || ENABLE_DO;
    bool secondary_needed = ENABLE_EC_TDS || ENABLE_ORP || ENABLE_TURBIDITY || ENABLE_CO2 || ENABLE_GAS_SENSOR;

    if (primary_needed) {
        ads_primary.begin(ADS1115_PRIMARY_ADDRESS);
    }
    if (secondary_needed) {
        ads_secondary.begin(ADS1115_SECONDARY_ADDRESS);
    }
}

// Runtime skip for disabled sensors / Bỏ qua cảm biến đã tắt trong config.
void initializeSensors() {
    for (auto* driver : sensor_drivers) {
        if (driver->isEnabled()) {
            driver->begin();
        }
    }
}

// Snapshot mapping / Ghi dữ liệu driver vào snapshot dùng chung.
void storeReading(const char* sensor_id, const SensorReading& reading) {
    if (strcmp(sensor_id, "temperature") == 0) {
        sensor_snapshot.temperature = reading;
    } else if (strcmp(sensor_id, "ph") == 0) {
        sensor_snapshot.ph = reading;
    } else if (strcmp(sensor_id, "do") == 0) {
        sensor_snapshot.dissolved_oxygen = reading;
    } else if (strcmp(sensor_id, "water_level") == 0) {
        sensor_snapshot.water_level = reading;
    } else if (strcmp(sensor_id, "ec") == 0) {
        sensor_snapshot.ec = reading;
    } else if (strcmp(sensor_id, "orp") == 0) {
        sensor_snapshot.orp = reading;
    } else if (strcmp(sensor_id, "turbidity") == 0) {
        sensor_snapshot.turbidity = reading;
    } else if (strcmp(sensor_id, "co2") == 0) {
        sensor_snapshot.co2 = reading;
    } else if (strcmp(sensor_id, "gas_sensor") == 0) {
        sensor_snapshot.gas_sensor = reading;
    }
}

// Generic polling / Đọc toàn bộ driver qua interface trừu tượng.
void readSensors() {
    for (auto* driver : sensor_drivers) {
        storeReading(driver->getId(), driver->read());
    }
}

// Auto rules / Chỉ áp dụng rule khi ở AUTO hoặc SAFE.
void applyRulesIfNeeded() {
    if (strcmp(current_mode, "AUTO") != 0 && strcmp(current_mode, "SAFE") != 0) {
        return;
    }

    const RuleEvaluation evaluation = rule_engine.evaluate(sensor_snapshot, *active_profile);
    relay_manager.setRelayState("aerator", evaluation.aerator);
    relay_manager.setRelayState("pump", evaluation.pump);
    relay_manager.setRelayState("circulation", evaluation.circulation);
    system_state = evaluation.system_state;
}
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Logger::info("Starting SmartFarm Aquaculture firmware / Khoi dong firmware SmartFarm Aquaculture");
    active_profile = getSpeciesProfile(DEFAULT_SPECIES_PROFILE);

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
    if (!wifi_manager.isConnected()) {
        wifi_manager.connect();
    }

    if (mqtt_manager.ensureConnected()) {
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
        mqtt_manager.publishSystemState(system_state, active_profile->id, current_mode);
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
