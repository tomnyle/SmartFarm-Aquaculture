#ifndef SRC_SENSORS_TURBIDITY_TURBIDITY_DRIVER_H
#define SRC_SENSORS_TURBIDITY_TURBIDITY_DRIVER_H

#include "../sensors.h"

class TurbidityDriver : public SensorDriver {
 public:
  explicit TurbidityDriver(bool enabled = false) : enabled_(enabled) {}
  bool begin() override { return false; }
  SensorReading read() override { return {SensorKind::TURBIDITY, "turbidity", 0.0f, false, millis()}; }
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
};

#endif
