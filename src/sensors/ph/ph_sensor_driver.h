#ifndef SRC_SENSORS_PH_PH_SENSOR_DRIVER_H
#define SRC_SENSORS_PH_PH_SENSOR_DRIVER_H

#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include "../sensors.h"

class PHSensorDriver : public SensorDriver {
 public:
  PHSensorDriver(uint8_t channel, bool enabled = true, uint8_t address = 0x48);
  bool begin() override;
  SensorReading read() override;
  bool isEnabled() const override { return enabled_; }

 private:
  bool enabled_;
  uint8_t channel_;
  uint8_t address_;
  Adafruit_ADS1115 ads_;
};

#endif
