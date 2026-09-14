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

// ==================== MQTT TOPICS (Home Assistant Format) ====================
// Sensor Topics - State
#define MQTT_TOPIC_WATER_TEMP HA_DISCOVERY_PREFIX "/sensor/aquaculture_water_temp/state"
#define MQTT_TOPIC_PH HA_DISCOVERY_PREFIX "/sensor/aquaculture_ph/state"
#define MQTT_TOPIC_TURBIDITY HA_DISCOVERY_PREFIX "/sensor/aquaculture_turbidity/state"
#define MQTT_TOPIC_DO HA_DISCOVERY_PREFIX "/sensor/aquaculture_do/state"
#define MQTT_TOPIC_CO2 HA_DISCOVERY_PREFIX "/sensor/aquaculture_co2/state"
#define MQTT_TOPIC_AIR_TEMP HA_DISCOVERY_PREFIX "/sensor/aquaculture_air_temp/state"
#define MQTT_TOPIC_HUMIDITY HA_DISCOVERY_PREFIX "/sensor/aquaculture_humidity/state"
#define MQTT_TOPIC_LIGHT HA_DISCOVERY_PREFIX "/sensor/aquaculture_light/state"

// Output Topics - State
#define MQTT_TOPIC_PUMP HA_DISCOVERY_PREFIX "/switch/aquaculture_pump/state"
#define MQTT_TOPIC_AERATOR HA_DISCOVERY_PREFIX "/switch/aquaculture_aerator/state"
#define MQTT_TOPIC_CIRCULATION HA_DISCOVERY_PREFIX "/switch/aquaculture_circulation/state"
#define MQTT_TOPIC_FEEDER HA_DISCOVERY_PREFIX "/switch/aquaculture_feeder/state"

// Control Topics - Command
#define MQTT_TOPIC_CONTROL_PUMP HA_DISCOVERY_PREFIX "/switch/aquaculture_pump/command"
#define MQTT_TOPIC_CONTROL_AERATOR HA_DISCOVERY_PREFIX "/switch/aquaculture_aerator/command"
#define MQTT_TOPIC_CONTROL_MODE HA_DISCOVERY_PREFIX "/select/aquaculture_mode/command"
#define MQTT_TOPIC_CONFIG_SPECIES HA_DISCOVERY_PREFIX "/select/aquaculture_species/command"

// Status Topic
#define MQTT_TOPIC_STATUS HA_DISCOVERY_PREFIX "/switch/aquaculture_status/state"
#define MQTT_TOPIC_STATE HA_DISCOVERY_PREFIX "/switch/aquaculture_controller/state"

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
