#ifndef SRC_CONFIG_RELAY_CONFIG_H
#define SRC_CONFIG_RELAY_CONFIG_H

#include <stdint.h>
#include "../../include/constants.h"

namespace relay_config {
constexpr bool ACTIVE_HIGH = true;
constexpr uint8_t RELAY_PINS[constants::RELAY_COUNT] = {12, 13, 14, 27, 26, 25, 33, 32};
}

#endif
