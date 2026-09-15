#ifndef SMARTFARM_APP_CONFIG_H
#define SMARTFARM_APP_CONFIG_H

#include <Arduino.h>

// Local override / File local không commit để giữ bí mật khi triển khai thực tế.
#if defined(__has_include)
#  if __has_include("app_config.local.h")
#    include "app_config.local.h"
#  endif
#endif

// Device identity / Định danh thiết bị.
#define FW_VERSION "1.0.0"
#define APP_DEVICE_ID "ESP32_AQUACULTURE_001"
#define APP_DEVICE_NAME "SmartFarm Aquaculture Controller"
#define APP_DEVICE_LOCATION "Pond A"

// WiFi and MQTT placeholders / Giá trị mẫu an toàn, cần thay khi triển khai thực tế.
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif
#ifndef MQTT_BROKER
#define MQTT_BROKER "192.168.1.10"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef MQTT_USER
#define MQTT_USER "mqtt_user"
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD "mqtt_password"
#endif
#define MQTT_CLIENT_ID APP_DEVICE_ID

// Shared timing / Chu kỳ xử lý chung.
#define WIFI_CONNECT_TIMEOUT_MS 30000UL
#define MQTT_RECONNECT_INTERVAL_MS 5000UL
#define SENSOR_READ_INTERVAL_MS 5000UL
#define MQTT_PUBLISH_INTERVAL_MS 10000UL
#define RULE_ENGINE_INTERVAL_MS 5000UL

// Hardware buses / Cấu hình bus phần cứng.
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define ADS1115_PRIMARY_ADDRESS 0x48
#define ADS1115_SECONDARY_ADDRESS 0x49
#define DS18B20_PIN 4
#define WATER_LEVEL_PIN 23
#define WATER_LEVEL_ACTIVE_STATE LOW

#endif
