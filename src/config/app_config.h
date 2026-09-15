#ifndef SRC_CONFIG_APP_CONFIG_H
#define SRC_CONFIG_APP_CONFIG_H

#include "../../include/constants.h"

namespace app_config {
constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char MQTT_HOST[] = "192.168.1.100";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_USERNAME[] = "mqtt_user";
constexpr char MQTT_PASSWORD[] = "mqtt_password";
constexpr char DEVICE_ID[] = "esp32-aquaculture-001";
constexpr char DEVICE_NAME[] = "SmartFarm Aquaculture";
constexpr char DEVICE_LOCATION[] = "Pond A";
constexpr char MQTT_BASE_TOPIC[] = "smartfarm/aquaculture";
constexpr char HA_DISCOVERY_PREFIX[] = "homeassistant";
constexpr char DEFAULT_PROFILE[] = "Shrimp";
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";
constexpr long GMT_OFFSET_SECONDS = 7 * 3600;
constexpr int DAYLIGHT_OFFSET_SECONDS = 0;
}

#endif
