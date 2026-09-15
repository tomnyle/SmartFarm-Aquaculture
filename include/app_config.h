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

// ==================== MQTT TOPICS ====================
#define MQTT_BASE_TOPIC "smartfarm/aquaculture/" FW_DEVICE_ID
#define MQTT_SENSOR_BASE MQTT_BASE_TOPIC "/sensor"
#define MQTT_RELAY_BASE MQTT_BASE_TOPIC "/relay"

// Sensor Topics - State
#define MQTT_TOPIC_WATER_TEMP MQTT_SENSOR_BASE "/water_temp"
#define MQTT_TOPIC_WATER_TEMP_MIN MQTT_SENSOR_BASE "/water_temp/min"
#define MQTT_TOPIC_WATER_TEMP_MAX MQTT_SENSOR_BASE "/water_temp/max"
#define MQTT_TOPIC_WATER_TEMP_STATUS MQTT_SENSOR_BASE "/water_temp/status"

#define MQTT_TOPIC_PH MQTT_SENSOR_BASE "/ph"
#define MQTT_TOPIC_PH_MIN MQTT_SENSOR_BASE "/ph/min"
#define MQTT_TOPIC_PH_MAX MQTT_SENSOR_BASE "/ph/max"
#define MQTT_TOPIC_PH_STATUS MQTT_SENSOR_BASE "/ph/status"

#define MQTT_TOPIC_DO MQTT_SENSOR_BASE "/do"
#define MQTT_TOPIC_DO_MIN MQTT_SENSOR_BASE "/do/min"
#define MQTT_TOPIC_DO_CRITICAL MQTT_SENSOR_BASE "/do/critical"
#define MQTT_TOPIC_DO_STATUS MQTT_SENSOR_BASE "/do/status"

#define MQTT_TOPIC_WATER_LEVEL MQTT_SENSOR_BASE "/water_level"
#define MQTT_TOPIC_WATER_LEVEL_MIN MQTT_SENSOR_BASE "/water_level/min"
#define MQTT_TOPIC_WATER_LEVEL_MAX MQTT_SENSOR_BASE "/water_level/max"
#define MQTT_TOPIC_WATER_LEVEL_STATUS MQTT_SENSOR_BASE "/water_level/status"

#define MQTT_TOPIC_TURBIDITY MQTT_SENSOR_BASE "/turbidity"
#define MQTT_TOPIC_TURBIDITY_MAX MQTT_SENSOR_BASE "/turbidity/max"
#define MQTT_TOPIC_TURBIDITY_STATUS MQTT_SENSOR_BASE "/turbidity/status"

#define MQTT_TOPIC_AIR_TEMP MQTT_SENSOR_BASE "/air_temp"
#define MQTT_TOPIC_AIR_TEMP_STATUS MQTT_SENSOR_BASE "/air_temp/status"

#define MQTT_TOPIC_HUMIDITY MQTT_SENSOR_BASE "/humidity"
#define MQTT_TOPIC_HUMIDITY_MIN MQTT_SENSOR_BASE "/humidity/min"
#define MQTT_TOPIC_HUMIDITY_MAX MQTT_SENSOR_BASE "/humidity/max"
#define MQTT_TOPIC_HUMIDITY_STATUS MQTT_SENSOR_BASE "/humidity/status"

#define MQTT_TOPIC_LIGHT MQTT_SENSOR_BASE "/light"
#define MQTT_TOPIC_LIGHT_MIN MQTT_SENSOR_BASE "/light/min"
#define MQTT_TOPIC_LIGHT_STATUS MQTT_SENSOR_BASE "/light/status"

#define MQTT_TOPIC_CO2 MQTT_SENSOR_BASE "/co2"
#define MQTT_TOPIC_CO2_MAX MQTT_SENSOR_BASE "/co2/max"
#define MQTT_TOPIC_CO2_STATUS MQTT_SENSOR_BASE "/co2/status"

// Relay Topics - State/Command
#define MQTT_TOPIC_PUMP_STATE MQTT_RELAY_BASE "/pump/state"
#define MQTT_TOPIC_PUMP_COMMAND MQTT_RELAY_BASE "/pump/command/set"
#define MQTT_TOPIC_AERATOR_STATE MQTT_RELAY_BASE "/aerator/state"
#define MQTT_TOPIC_AERATOR_COMMAND MQTT_RELAY_BASE "/aerator/command/set"
#define MQTT_TOPIC_CIRCULATION_STATE MQTT_RELAY_BASE "/circulation/state"
#define MQTT_TOPIC_CIRCULATION_COMMAND MQTT_RELAY_BASE "/circulation/command/set"
#define MQTT_TOPIC_FEEDER_STATE MQTT_RELAY_BASE "/feeder/state"
#define MQTT_TOPIC_FEEDER_COMMAND MQTT_RELAY_BASE "/feeder/command/set"
#define MQTT_TOPIC_SPARE1_STATE MQTT_RELAY_BASE "/spare1/state"
#define MQTT_TOPIC_SPARE1_COMMAND MQTT_RELAY_BASE "/spare1/command/set"
#define MQTT_TOPIC_SPARE2_STATE MQTT_RELAY_BASE "/spare2/state"
#define MQTT_TOPIC_SPARE2_COMMAND MQTT_RELAY_BASE "/spare2/command/set"

// Mode/Profile topics
#define MQTT_TOPIC_MODE_STATE MQTT_BASE_TOPIC "/mode"
#define MQTT_TOPIC_MODE_SET MQTT_BASE_TOPIC "/mode/set"
#define MQTT_TOPIC_PROFILE_STATE MQTT_BASE_TOPIC "/profile"
#define MQTT_TOPIC_PROFILE_SET MQTT_BASE_TOPIC "/profile/set"

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

// Water level (%)
#define WATER_LEVEL_ALERT_LOW 20
#define WATER_LEVEL_ALERT_HIGH 100

// Turbidity (NTU)
#define TURBIDITY_ALERT_HIGH 5.0

// Humidity (%)
#define HUMIDITY_ALERT_LOW 50
#define HUMIDITY_ALERT_HIGH 80

// Light (lux)
#define LIGHT_ALERT_LOW 500

// ==================== OUTPUT CONTROL ====================
// Auto control conditions
#define AUTO_PUMP_TEMP_HIGH 30       // Turn on pump if temp > this
#define AUTO_PUMP_DO_LOW 5.0         // Turn on pump if DO < this
#define AUTO_PUMP_CO2_HIGH 5.0       // Turn on pump if CO2 > this
#define AUTO_AERATOR_DO_LOW 4.0      // Turn on aerator if DO < this
#define AUTO_CIRCULATION_TEMP_HIGH 28 // Turn on circulation if temp > this

// Pump minimum on time
#define PUMP_MIN_ON_TIME 10000  // 10 seconds
#define PUMP_MIN_OFF_TIME 30000 // 30 seconds

#endif // APP_CONFIG_H
