#ifndef SRC_SENSORS_DISSOLVED_OXYGEN_DO_SENSOR_DRIVER_H
#define SRC_SENSORS_DISSOLVED_OXYGEN_DO_SENSOR_DRIVER_H

#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include "../sensors.h"

class DOSensorDriver : public SensorDriver {
 public:
  DOSensorDriver(Adafruit_ADS1115& ads, uint8_t channel, bool enabled = true);
  bool begin() override;
  SensorReading read() override;
  bool isEnabled() const override { return enabled_; }
  void setInitialized(bool initialized) { initialized_ = initialized; }

 private:
  Adafruit_ADS1115& ads_;
  bool enabled_;
  bool initialized_ = false;
  uint8_t channel_;
};

#endif
