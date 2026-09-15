#include "do_sensor_driver.h"

DOSensorDriver::DOSensorDriver(Adafruit_ADS1115& ads, uint8_t channel, bool enabled)
    : ads_(ads), enabled_(enabled), channel_(channel) {}

bool DOSensorDriver::begin() {
  return enabled_ && initialized_;
}

SensorReading DOSensorDriver::read() {
  SensorReading reading{SensorKind::DISSOLVED_OXYGEN, "do", 0.0f, false, millis()};
  if (!enabled_ || !initialized_) return reading;
  const int16_t raw = ads_.readADC_SingleEnded(channel_);
  const float voltage = raw * 0.1875f / 1000.0f;
  const float value = voltage * 4.0f;
  reading.value = value;
  reading.available = value >= 0.0f && value <= 20.0f;
  return reading;
}
