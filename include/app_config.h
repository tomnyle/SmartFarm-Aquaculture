#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// Firmware Version
#define FW_VERSION "0.3.0"
#define FW_BUILD_DATE __DATE__
#define FW_DEVICE_ID "ESP32_AQUACULTURE_001"

// ==================== WIFI CONFIGURATION ====================
// Replace with your local Wi-Fi credentials before flashing.
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds

// ==================== MQTT CONFIGURATION ====================
// Replace with your local MQTT broker settings before flashing.
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USER "your_mqtt_username"
#define MQTT_PASSWORD "your_mqtt_password"
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
#define MQTT_TOPIC_CONTROL_OPERATION_PROFILE "smartfarm/aquaculture/config/operation_profile/set"
#define MQTT_TOPIC_CONTROL_LIVESTOCK_PRESENT "smartfarm/aquaculture/config/livestock_present/set"
#define MQTT_TOPIC_CONTROL_RELAY_TEST "smartfarm/aquaculture/config/relay_test/set"

// Select state topics
#define MQTT_TOPIC_MODE_STATE "smartfarm/aquaculture/config/mode/state"
#define MQTT_TOPIC_SPECIES_STATE "smartfarm/aquaculture/config/species/state"
#define MQTT_TOPIC_OPERATION_PROFILE_SELECTED "smartfarm/aquaculture/config/operation_profile/selected"
#define MQTT_TOPIC_OPERATION_PROFILE_ACTUAL "smartfarm/aquaculture/config/operation_profile/actual"
#define MQTT_TOPIC_LIVESTOCK_PRESENT_STATE "smartfarm/aquaculture/config/livestock_present/state"
#define MQTT_TOPIC_RELAY_TEST_STATE "smartfarm/aquaculture/config/relay_test/state"

// Status Topic
#define MQTT_TOPIC_STATUS "smartfarm/aquaculture/status"
#define MQTT_TOPIC_STATE "smartfarm/aquaculture/controller/state"
#define MQTT_TOPIC_CAN_NO_LOAD_TEST "smartfarm/aquaculture/eligibility/can_no_load_test"
#define MQTT_TOPIC_CAN_PRODUCTION "smartfarm/aquaculture/eligibility/can_production"
#define MQTT_TOPIC_OUTPUTS_LOCKED "smartfarm/aquaculture/eligibility/outputs_locked"
#define MQTT_TOPIC_REQUIRED_SENSORS_VALID "smartfarm/aquaculture/eligibility/required_sensors_valid"
#define MQTT_TOPIC_SENSOR_FAULT_ACTIVE "smartfarm/aquaculture/eligibility/sensor_fault_active"
#define MQTT_TOPIC_ANY_ACTIVE_ALARM "smartfarm/aquaculture/eligibility/any_active_alarm"
#define MQTT_TOPIC_CRITICAL_CONDITION_ACTIVE "smartfarm/aquaculture/eligibility/critical_condition_active"
#define MQTT_TOPIC_PROFILE_BLOCK_CODES "smartfarm/aquaculture/eligibility/block_reason_codes"
#define MQTT_TOPIC_PROFILE_BLOCK_SUMMARY "smartfarm/aquaculture/eligibility/block_reason_summary"
#define MQTT_TOPIC_PRODUCTION_BLOCK_SUMMARY "smartfarm/aquaculture/eligibility/production_block_reason"
#define MQTT_TOPIC_ACTIVE_REMINDER "smartfarm/aquaculture/eligibility/active_reminder"
#define MQTT_TOPIC_BLOCK_REQUIRED_SENSOR_INVALID "smartfarm/aquaculture/eligibility/blockers/required_sensor_invalid"
#define MQTT_TOPIC_BLOCK_SENSOR_FAULT_ACTIVE "smartfarm/aquaculture/eligibility/blockers/sensor_fault_active"
#define MQTT_TOPIC_BLOCK_RELAY_TEST_NOT_PASSED "smartfarm/aquaculture/eligibility/blockers/relay_test_not_passed"
#define MQTT_TOPIC_BLOCK_OUTPUTS_LOCKED "smartfarm/aquaculture/eligibility/blockers/outputs_locked"
#define MQTT_TOPIC_BLOCK_SENSOR_TEST_MODE_ENABLED "smartfarm/aquaculture/eligibility/blockers/sensor_test_mode_enabled"
#define MQTT_TOPIC_BLOCK_ACTIVE_ALARM "smartfarm/aquaculture/eligibility/blockers/active_alarm"
#define MQTT_TOPIC_BLOCK_LIVESTOCK_NOT_CONFIRMED "smartfarm/aquaculture/eligibility/blockers/livestock_not_confirmed"
#define MQTT_TOPIC_BLOCK_CRITICAL_CONDITION_ACTIVE "smartfarm/aquaculture/eligibility/blockers/critical_condition_active"

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

// ==================== SAFETY PROFILE LOCKS ====================
// Keep true during sensor-only checkout; ON commands are suppressed,
// outputs remain locked OFF, and AUTO rules are skipped.
#define SENSOR_TEST_MODE true

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