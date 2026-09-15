#ifndef SRC_SENSORS_EC_TDS_EC_TDS_DRIVER_H
#define SRC_SENSORS_EC_TDS_EC_TDS_DRIVER_H

#include "../sensors.h"

class ECTDSDriver : public SensorDriver {
 public:
  explicit ECTDSDriver(bool enabled = false) : enabled_(enabled) {}
  bool begin() override { return false; }
  SensorReading read() override { return {SensorKind::EC_TDS, "ec", 0.0f, false, millis()}; }
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
};

#endif
