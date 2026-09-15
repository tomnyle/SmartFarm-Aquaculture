#ifndef SRC_SENSORS_GAS_SENSOR_GAS_SENSOR_DRIVER_H
#define SRC_SENSORS_GAS_SENSOR_GAS_SENSOR_DRIVER_H

#include "../sensors.h"

class GasSensorDriver : public SensorDriver {
 public:
  explicit GasSensorDriver(bool enabled = false) : enabled_(enabled) {}
  bool begin() override { return false; }
  SensorReading read() override { return {SensorKind::GAS_SENSOR, "gas_sensor", 0.0f, false, millis()}; }
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
};

#endif
