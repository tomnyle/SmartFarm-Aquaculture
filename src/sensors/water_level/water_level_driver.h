#ifndef SRC_SENSORS_WATER_LEVEL_WATER_LEVEL_DRIVER_H
#define SRC_SENSORS_WATER_LEVEL_WATER_LEVEL_DRIVER_H

#include "../sensors.h"

class WaterLevelDriver : public SensorDriver {
 public:
  explicit WaterLevelDriver(uint8_t pin, bool enabled = true);
  bool begin() override;
  SensorReading read() override;
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
  uint8_t pin_;
};

#endif
