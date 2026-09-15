#ifndef SMARTFARM_WATER_LEVEL_DRIVER_H
#define SMARTFARM_WATER_LEVEL_DRIVER_H

#include "../../config/sensors_config.h"
#include "../sensors.h"

class WaterLevelDriver : public SensorDriver {
public:
    explicit WaterLevelDriver(const WaterLevelSensorConfig& config);

    bool begin() override;
    SensorReading read() override;
    bool isEnabled() const override;
    const char* getId() const override;

private:
    WaterLevelSensorConfig config_;
    uint16_t error_count_;
};

#endif
