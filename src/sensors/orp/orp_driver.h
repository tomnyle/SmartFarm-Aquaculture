#ifndef SRC_SENSORS_ORP_ORP_DRIVER_H
#define SRC_SENSORS_ORP_ORP_DRIVER_H

#include "../sensors.h"

class ORPDriver : public SensorDriver {
 public:
  explicit ORPDriver(bool enabled = false) : enabled_(enabled) {}
  bool begin() override { return false; }
  SensorReading read() override { return {SensorKind::ORP, "orp", 0.0f, false, millis()}; }
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
};

#endif
