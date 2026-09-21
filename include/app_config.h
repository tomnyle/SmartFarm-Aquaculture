#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// Firmware Version
#define FW_VERSION "0.2.0"
#define FW_BUILD_DATE __DATE__
#define FW_DEVICE_ID "ESP32_AQUACULTURE_001"

// ==================== WIFI CONFIGURATION ====================
#define WIFI_SSID "Le Danh"
#define WIFI_PASSWORD "123456789"
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds

// ==================== MQTT CONFIGURATION ====================
#define MQTT_BROKER "192.168.100.168"
#define MQTT_PORT 1883
#define MQTT_USER "homer"
#define MQTT_PASSWORD "Danh@@@1992"
#define MQTT_CLIENT_ID "ESP32_AQUACULTURE"
#define MQTT_RECONNECT_INTERVAL 5000

// ==================== HOME ASSISTANT MQTT DISCOVERY ====================
#define HA_DISCOVERY_PREFIX "homeassistant"
#define HA_DISCOVERY_ENABLED true

// ==================== MQTT TOPICS (APPLICATION NAMESPACE) ====================
// Sensor Topics - State
#define MQTT_TOPIC_WATER_TEMP "smartfarm/aquaculture/sensor/water_temp"
#define MQTT_TOPIC_PH "smartfarm/aquaculture/sensor/ph"
#define MQTT_TOPIC_TURBIDITY "smartfarm/aquaculture/sensor/turbidity"
#define MQTT_TOPIC_DO "smartfarm/aquaculture/sensor/do"
#define MQTT_TOPIC_CO2 "smartfarm/aquaculture/sensor/co2"
#define MQTT_TOPIC_AIR_TEMP "smartfarm/aquaculture/sensor/air_temp"
#define MQTT_TOPIC_HUMIDITY "smartfarm/aquaculture/sensor/humidity"
#define MQTT_TOPIC_LIGHT "smartfarm/aquaculture/sensor/light"

// Output Topics - State
#define MQTT_TOPIC_PUMP "smartfarm/aquaculture/output/pump"
#define MQTT_TOPIC_AERATOR "smartfarm/aquaculture/output/aerator"
#define MQTT_TOPIC_CIRCULATION "smartfarm/aquaculture/output/circulation"
#define MQTT_TOPIC_FEEDER "smartfarm/aquaculture/output/feeder"

// Control Topics - Command
#define MQTT_TOPIC_CONTROL_PUMP "smartfarm/aquaculture/control/pump/set"
#define MQTT_TOPIC_CONTROL_AERATOR "smartfarm/aquaculture/control/aerator/set"
#define MQTT_TOPIC_CONTROL_CIRCULATION "smartfarm/aquaculture/control/circulation/set"
#define MQTT_TOPIC_CONTROL_FEEDER "smartfarm/aquaculture/control/feeder/set"
#define MQTT_TOPIC_CONTROL_MODE "smartfarm/aquaculture/config/mode/set"
#define MQTT_TOPIC_CONFIG_SPECIES "smartfarm/aquaculture/config/species/set"

// Select state topics
#define MQTT_TOPIC_MODE_STATE "smartfarm/aquaculture/config/mode/state"
#define MQTT_TOPIC_SPECIES_STATE "smartfarm/aquaculture/config/species/state"

// Status Topic
#define MQTT_TOPIC_STATUS "smartfarm/aquaculture/status"
#define MQTT_TOPIC_STATE "smartfarm/aquaculture/controller/state"

// Production monitoring topics - conditions
#define MQTT_TOPIC_COND_TEMP_HIGH "smartfarm/aquaculture/condition/water_temp_high"
#define MQTT_TOPIC_COND_TEMP_LOW "smartfarm/aquaculture/condition/water_temp_low"
#define MQTT_TOPIC_COND_PH_LOW "smartfarm/aquaculture/condition/ph_low"
#define MQTT_TOPIC_COND_PH_HIGH "smartfarm/aquaculture/condition/ph_high"
#define MQTT_TOPIC_COND_DO_LOW "smartfarm/aquaculture/condition/do_low"
#define MQTT_TOPIC_COND_DO_CRITICAL "smartfarm/aquaculture/condition/do_critical"
#define MQTT_TOPIC_COND_CO2_HIGH "smartfarm/aquaculture/condition/co2_high"
#define MQTT_TOPIC_COND_TURBIDITY_HIGH "smartfarm/aquaculture/condition/turbidity_high"
#define MQTT_TOPIC_COND_SENSOR_FAULT "smartfarm/aquaculture/condition/sensor_fault"
#define MQTT_TOPIC_COND_ANY_ACTIVE "smartfarm/aquaculture/condition/any_active"
#define MQTT_TOPIC_COND_OUTPUTS_LOCKED "smartfarm/aquaculture/condition/outputs_locked"
#define MQTT_TOPIC_RELAY_TEST_STATUS "smartfarm/aquaculture/condition/relay_test_status"

// Production monitoring topics - process
#define MQTT_TOPIC_PRODUCTION_PHASE "smartfarm/aquaculture/process/phase"
#define MQTT_TOPIC_PRODUCTION_READY "smartfarm/aquaculture/process/readiness"
#define MQTT_TOPIC_PROCESS_SUMMARY "smartfarm/aquaculture/process/summary"

// ==================== DEVICE CONFIGURATION ====================
#define DEVICE_NAME "Aquaculture-Controller-001"
#define DEVICE_LOCATION "Home Pond"

// ==================== SENSOR READ INTERVALS ====================
#define SENSOR_READ_INTERVAL 5000      // 5 seconds
#define MQTT_PUBLISH_INTERVAL 10000    // 10 seconds
#define RULE_ENGINE_INTERVAL 5000      // 5 seconds

// ==================== OTA UPDATE ====================
#define OTA_ENABLED false
#define OTA_PORT 3232

// ==================== WATCHDOG TIMER ====================
#define WATCHDOG_ENABLED true
#define WATCHDOG_TIMEOUT 30000  // 30 seconds

// ==================== LOGGING ====================
#define LOGGING_ENABLED true
#define LOG_LEVEL 3  // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG

// ==================== DEFAULT MODE ====================
#define DEFAULT_MODE "AUTO"  // AUTO, MANUAL, SCHEDULE, SAFE

// ==================== TEST/SAFETY MODE ====================
#define BENCH_TEST_MODE false
// Keep false for default production builds; set true during hardware bring-up.
#define SENSOR_TEST_MODE false
#if BENCH_TEST_MODE && SENSOR_TEST_MODE
#error "BENCH_TEST_MODE and SENSOR_TEST_MODE cannot both be true"
#endif

// ==================== OPTIONAL SENSOR FLAGS ====================
#define CO2_SENSOR_ENABLED true
#define LIGHT_SENSOR_ENABLED true

// ==================== CONTROL THRESHOLDS ====================
// Temperature (°C)
#define TEMP_ALERT_HIGH 30
#define TEMP_ALERT_LOW 15

// pH
#define PH_ALERT_HIGH 8.0
#define PH_ALERT_LOW 6.5

// Dissolved Oxygen (mg/L)
#define DO_ALERT_LOW 5.0
#define DO_CRITICAL_LOW 3.0

// CO2 (ppm)
#define CO2_ALERT_HIGH 5.0

// ==================== OUTPUT CONTROL ====================
// Auto control conditions
#define AUTO_PUMP_TEMP_HIGH 30
#define AUTO_PUMP_DO_LOW 5.0
#define AUTO_PUMP_CO2_HIGH 5.0
#define AUTO_AERATOR_DO_LOW 4.0
#define AUTO_CIRCULATION_TEMP_HIGH 28

// Pump minimum on time
#define PUMP_MIN_ON_TIME 10000
#define PUMP_MIN_OFF_TIME 30000

#endif // APP_CONFIG_H