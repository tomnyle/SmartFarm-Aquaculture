#ifndef SMARTFARM_SENSORS_H
#define SMARTFARM_SENSORS_H

#include "../../include/types.h"

class SensorDriver {
public:
    virtual ~SensorDriver() = default;
    virtual bool begin() = 0;
    virtual SensorReading read() = 0;
    virtual bool isEnabled() const = 0;
    virtual const char* getId() const = 0;
};

#endif
