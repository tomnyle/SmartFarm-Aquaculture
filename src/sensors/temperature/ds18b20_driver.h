#ifndef SRC_SENSORS_TEMPERATURE_DS18B20_DRIVER_H
#define SRC_SENSORS_TEMPERATURE_DS18B20_DRIVER_H

#include <DallasTemperature.h>
#include <OneWire.h>
#include "../sensors.h"

class DS18B20Driver : public SensorDriver {
 public:
  explicit DS18B20Driver(uint8_t pin, bool enabled = true);
  bool begin() override;
  SensorReading read() override;
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
  OneWire oneWire_;
  DallasTemperature sensor_;
};

#endif
