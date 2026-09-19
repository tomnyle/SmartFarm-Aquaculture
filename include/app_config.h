#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// Firmware Version
#define FW_VERSION "0.3.0"
#define FW_BUILD_DATE __DATE__
#define FW_DEVICE_ID "ESP32_AQUACULTURE_001"

// ==================== WIFI CONFIGURATION ====================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds

// ==================== MQTT CONFIGURATION ====================
#define MQTT_BROKER "YOUR_MQTT_BROKER"
#define MQTT_PORT 1883
#define MQTT_USER "YOUR_MQTT_USER"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
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
#define MQTT_TOPIC_WATER_LEVEL "smartfarm/aquaculture/sensor/water_level"
#define MQTT_TOPIC_AERATOR_CURRENT "smartfarm/aquaculture/sensor/aerator_current"
#define MQTT_TOPIC_PUMP_CURRENT "smartfarm/aquaculture/sensor/pump_current"

// Output Topics - State
#define MQTT_TOPIC_PUMP "smartfarm/aquaculture/output/pump"
#define MQTT_TOPIC_AERATOR_1 "smartfarm/aquaculture/output/aerator_1"
#define MQTT_TOPIC_AERATOR_2 "smartfarm/aquaculture/output/aerator_2"
#define MQTT_TOPIC_FEEDER "smartfarm/aquaculture/output/feeder"
#define MQTT_TOPIC_ALARM_OUTPUT "smartfarm/aquaculture/output/alarm"
#define MQTT_TOPIC_CIRCULATION "smartfarm/aquaculture/output/circulation"

// Backward-compatible aliases
#define MQTT_TOPIC_AERATOR "smartfarm/aquaculture/output/aerator"

// Control Topics - Command
#define MQTT_TOPIC_CONTROL_PUMP "smartfarm/aquaculture/control/pump/set"
#define MQTT_TOPIC_CONTROL_AERATOR "smartfarm/aquaculture/control/aerator/set"
#define MQTT_TOPIC_CONTROL_AERATOR_1 "smartfarm/aquaculture/control/aerator_1/set"
#define MQTT_TOPIC_CONTROL_AERATOR_2 "smartfarm/aquaculture/control/aerator_2/set"
#define MQTT_TOPIC_CONTROL_CIRCULATION "smartfarm/aquaculture/control/circulation/set"
#define MQTT_TOPIC_CONTROL_FEEDER "smartfarm/aquaculture/control/feeder/set"
#define MQTT_TOPIC_CONTROL_ALARM "smartfarm/aquaculture/control/alarm/set"
#define MQTT_TOPIC_CONTROL_MODE "smartfarm/aquaculture/config/mode/set"
#define MQTT_TOPIC_CONFIG_SPECIES "smartfarm/aquaculture/config/species/set"

// Select state topics
#define MQTT_TOPIC_MODE_STATE "smartfarm/aquaculture/config/mode/state"
#define MQTT_TOPIC_SPECIES_STATE "smartfarm/aquaculture/config/species/state"

// Event / Status Topics
#define MQTT_TOPIC_STATUS "smartfarm/aquaculture/status"
#define MQTT_TOPIC_STATUS_HEARTBEAT "smartfarm/aquaculture/status/heartbeat"
#define MQTT_TOPIC_STATUS_SAFETY_STATE "smartfarm/aquaculture/status/safety_state"
#define MQTT_TOPIC_STATUS_ALARM_TEXT "smartfarm/aquaculture/status/alarm_text"
#define MQTT_TOPIC_STATUS_ALARM_ACTIVE "smartfarm/aquaculture/status/alarm_active"
#define MQTT_TOPIC_STATUS_SAFETY_ACTIVE "smartfarm/aquaculture/status/safety_active"
#define MQTT_TOPIC_STATUS_EMERGENCY_ACTIVE "smartfarm/aquaculture/status/emergency_active"
#define MQTT_TOPIC_STATE "smartfarm/aquaculture/controller/state"
#define MQTT_TOPIC_EVENT_ALARM "smartfarm/aquaculture/event/alarm"
#define MQTT_TOPIC_EVENT_SENSOR_FAULT "smartfarm/aquaculture/event/sensor_fault"
#define MQTT_TOPIC_EVENT_DEVICE_FAULT "smartfarm/aquaculture/event/device_fault"

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
#define DEFAULT_MODE "AUTO"  // AUTO, MANUAL, SCHEDULE, SAFE, EMERGENCY

// ==================== V1 SAFETY THRESHOLDS ====================
#define DO_WARNING_THRESHOLD 5.5f
#define DO_LOW_THRESHOLD 4.5f
#define DO_CRITICAL_THRESHOLD 3.5f
#define DO_EMERGENCY_THRESHOLD 2.5f
#define DO_RECOVERY_THRESHOLD 6.0f

#define PH_LOW_WARNING 6.5f
#define PH_HIGH_WARNING 8.5f
#define PH_RATE_LIMIT 0.5f  // pH units per hour

#define TEMP_WARNING 31.0f
#define TEMP_CRITICAL 33.0f

#define WATER_LEVEL_LOW 35.0f
#define WATER_LEVEL_CRITICAL 20.0f
#define WATER_LEVEL_RECOVERY 60.0f

#define SENSOR_STALE_TIMEOUT_MS 30000UL
#define PUMP_MAX_RUN_TIME_MS 900000UL
#define FEEDER_MAX_RUN_TIME_MS 20000UL
#define MANUAL_OVERRIDE_TIMEOUT_MS 1800000UL

#define CURRENT_MIN_RUNNING_A 0.20f
#define CURRENT_MAX_RUNNING_A 5.00f

// ==================== OUTPUT CONTROL ====================
#define PUMP_MIN_ON_TIME 10000UL
#define PUMP_MIN_OFF_TIME 30000UL
#define OUTPUT_MIN_CHANGE_INTERVAL_MS 5000UL
#define SAFETY_RECOVERY_HOLD_MS 60000UL
#define CURRENT_FAULT_CONFIRM_MS 15000UL
#define CURRENT_SENSOR_FULL_SCALE_A 5.0f

#endif // APP_CONFIG_H
