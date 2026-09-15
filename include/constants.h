#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

namespace constants {
constexpr uint8_t RELAY_COUNT = 8;
constexpr uint8_t MAX_SCHEDULE_ENTRIES = 5;
constexpr uint16_t SENSOR_READ_INTERVAL_MS = 5000;
constexpr uint16_t RULE_EVALUATION_INTERVAL_MS = 5000;
constexpr uint16_t MQTT_PUBLISH_INTERVAL_MS = 5000;
constexpr uint16_t WIFI_RETRY_DELAY_MS = 500;
}

#endif
