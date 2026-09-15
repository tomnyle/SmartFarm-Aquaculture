#ifndef SMARTFARM_DS18B20_DRIVER_H
#define SMARTFARM_DS18B20_DRIVER_H

#include <DallasTemperature.h>
#include <OneWire.h>
#include "../../config/sensors_config.h"
#include "../sensors.h"

class DS18B20Driver : public SensorDriver {
public:
    explicit DS18B20Driver(const TemperatureSensorConfig& config);

    bool begin() override;
    SensorReading read() override;
    bool isEnabled() const override;
    const char* getId() const override;

private:
    TemperatureSensorConfig config_;
    OneWire one_wire_;
    DallasTemperature sensor_bus_;
    uint16_t error_count_;
};

#endif
