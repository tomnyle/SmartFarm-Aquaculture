#include "ph_sensor_driver.h"

PHSensorDriver::PHSensorDriver(uint8_t channel, bool enabled, uint8_t address)
    : enabled_(enabled), channel_(channel), address_(address) {}

bool PHSensorDriver::begin() {
  if (!enabled_) return false;
  const bool ready = ads_.begin(address_);
  if (ready) {
    ads_.setGain(GAIN_TWOTHIRDS);
  }
  return ready;
}

SensorReading PHSensorDriver::read() {
  SensorReading reading{SensorKind::PH, "ph", 0.0f, false, millis()};
  if (!enabled_) return reading;
  const int16_t raw = ads_.readADC_SingleEnded(channel_);
  const float voltage = raw * 0.1875f / 1000.0f;
  const float ph = 7.0f + ((2.5f - voltage) / 0.18f);
  reading.value = ph;
  reading.available = ph >= 0.0f && ph <= 14.0f;
  return reading;
}
