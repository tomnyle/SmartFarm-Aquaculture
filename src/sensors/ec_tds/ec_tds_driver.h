#ifndef SMARTFARM_EC_TDS_DRIVER_H
#define SMARTFARM_EC_TDS_DRIVER_H

#include <Adafruit_ADS1X15.h>
#include "../../config/sensors_config.h"
#include "../sensors.h"

class ECTDSDriver : public SensorDriver {
public:
    ECTDSDriver(Adafruit_ADS1115* ads, const ADS1115SensorConfig& config) : ads_(ads), config_(config) {}
    bool begin() override { return config_.base.enabled && ads_ != nullptr; }
    SensorReading read() override { return {millis(), 0.0F, config_.base.enabled ? SENSOR_STATUS_ERROR : SENSOR_STATUS_DISABLED, 0}; }
    bool isEnabled() const override { return config_.base.enabled; }
    const char* getId() const override { return config_.base.id; }
    uint8_t getBusIndex() const override { return config_.ads_index; }

private:
    Adafruit_ADS1115* ads_;
    ADS1115SensorConfig config_;
};

#endif
