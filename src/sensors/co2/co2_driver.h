#ifndef SRC_SENSORS_CO2_CO2_DRIVER_H
#define SRC_SENSORS_CO2_CO2_DRIVER_H

#include "../sensors.h"

class CO2Driver : public SensorDriver {
 public:
  explicit CO2Driver(bool enabled = false) : enabled_(enabled) {}
  bool begin() override { return false; }
  SensorReading read() override { return {SensorKind::CO2, "co2", 0.0f, false, millis()}; }
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
};

#endif
