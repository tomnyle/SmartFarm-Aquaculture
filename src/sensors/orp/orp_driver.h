#ifndef SMARTFARM_ORP_DRIVER_H
#define SMARTFARM_ORP_DRIVER_H

#include "../../config/sensors_config.h"
#include "../sensors.h"

class ORPDriver : public SensorDriver {
public:
    explicit ORPDriver(const ADS1115SensorConfig& config) : config_(config) {}
    bool begin() override { return false; }
    SensorReading read() override { return {millis(), 0.0F, config_.base.enabled ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0}; }
    bool isEnabled() const override { return config_.base.enabled; }
    const char* getId() const override { return config_.base.id; }

private:
    ADS1115SensorConfig config_;
};

#endif
