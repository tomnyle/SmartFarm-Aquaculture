#ifndef SMARTFARM_CONDITIONS_H
#define SMARTFARM_CONDITIONS_H

#include "../../include/types.h"

inline bool isSensorOk(const SensorReading& reading) {
    return reading.status == SENSOR_STATUS_OK;
}

inline bool isSensorEnabled(const SensorReading& reading) {
    return reading.status != SENSOR_STATUS_DISABLED;
}

inline bool isBelow(const SensorReading& reading, float threshold) {
    return isSensorOk(reading) && reading.value < threshold;
}

inline bool isAbove(const SensorReading& reading, float threshold) {
    return isSensorOk(reading) && reading.value > threshold;
}

#endif
