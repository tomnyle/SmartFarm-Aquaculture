#ifndef SMARTFARM_RELAY_CONFIG_H
#define SMARTFARM_RELAY_CONFIG_H

#include <Arduino.h>
#include "../../include/types.h"

struct RelayConfigEntry {
    const char* name;
    uint8_t pin;
    bool enabled;
    bool active_high;
    RelayState default_state;
};

static const uint8_t RELAY_COUNT = 8;

static const RelayConfigEntry RELAY_CONFIGS[RELAY_COUNT] = {
    {"aerator", 12, true, true, RELAY_OFF},
    {"pump", 13, true, true, RELAY_OFF},
    {"circulation", 14, true, true, RELAY_OFF},
    {"feeder", 27, true, true, RELAY_OFF},
    {"valve", 26, true, true, RELAY_OFF},
    {"light", 25, true, true, RELAY_OFF},
    {"spare_1", 33, true, true, RELAY_OFF},
    {"spare_2", 32, true, true, RELAY_OFF}
};

#endif
