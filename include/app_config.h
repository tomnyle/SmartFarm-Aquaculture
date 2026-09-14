#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// Firmware Version
#define FW_VERSION "0.1.0"
#define FW_BUILD_DATE __DATE__

// WiFi Configuration
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "aquaculture-esp32-001"
#define MQTT_RECONNECT_INTERVAL 5000

// MQTT Topics
#define MQTT_TOPIC_BASE "smartfarm/aquaculture"
#define MQTT_TOPIC_STATE MQTT_TOPIC_BASE "/state"
#define MQTT_TOPIC_SENSOR MQTT_TOPIC_BASE "/sensor"
#define MQTT_TOPIC_OUTPUT MQTT_TOPIC_BASE "/output"
#define MQTT_TOPIC_CONTROL MQTT_TOPIC_BASE "/control"
#define MQTT_TOPIC_CONFIG MQTT_TOPIC_BASE "/config"
#define MQTT_TOPIC_STATUS MQTT_TOPIC_BASE "/status"

// Device Configuration
#define DEVICE_NAME "Aquaculture-Controller-001"
#define DEVICE_LOCATION "Home Pond"

// OTA Update
#define OTA_ENABLED true
#define OTA_PORT 3232

// Watchdog Timer
#define WATCHDOG_ENABLED true
#define WATCHDOG_TIMEOUT 30000  // 30 seconds

// Logging
#define LOGGING_ENABLED true
#define LOG_LEVEL 3  // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG

// System State
#define SYSTEM_STATE_SAVE_INTERVAL 60000  // Save every 60 seconds

// Default Mode
#define DEFAULT_MODE "AUTO"  // AUTO, MANUAL, SCHEDULE, SAFE

// Sensor Read Intervals (milliseconds)
#define SENSOR_READ_INTERVAL 30000  // 30 seconds
#define SENSOR_CALIBRATION_INTERVAL 86400000  // 24 hours

// Rule Engine
#define RULE_ENGINE_INTERVAL 5000  // 5 seconds
#define RULE_ENGINE_DEBOUNCE 2000  // 2 seconds

#endif // APP_CONFIG_H
