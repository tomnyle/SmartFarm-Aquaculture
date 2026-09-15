#ifndef SMARTFARM_DO_SENSOR_DRIVER_H
#define SMARTFARM_DO_SENSOR_DRIVER_H

#include <Adafruit_ADS1X15.h>
#include "../../config/sensors_config.h"
#include "../../utils/calibration.h"
#include "../sensors.h"

class DOSensorDriver : public SensorDriver {
public:
    DOSensorDriver(Adafruit_ADS1115* ads, const ADS1115SensorConfig& config);

    bool begin() override;
    SensorReading read() override;
    bool isEnabled() const override;
    const char* getId() const override;

private:
    float toVoltage(int16_t raw) const;

    Adafruit_ADS1115* ads_;
    ADS1115SensorConfig config_;
    uint16_t error_count_;
};

#endif
