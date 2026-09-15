#include <Arduino.h>
#include "water_level_driver.h"

WaterLevelDriver::WaterLevelDriver(uint8_t pin, bool enabled) : enabled_(enabled), pin_(pin) {}

bool WaterLevelDriver::begin() {
  if (!enabled_) return false;
  pinMode(pin_, INPUT);
  return true;
}

SensorReading WaterLevelDriver::read() {
  SensorReading reading{SensorKind::WATER_LEVEL, "water_level", 0.0f, false, millis()};
  if (!enabled_) return reading;
  const int raw = analogRead(pin_);
  const float percent = (static_cast<float>(raw) / 4095.0f) * 100.0f;
  reading.value = percent;
  reading.available = percent >= 0.0f && percent <= 100.0f;
  return reading;
}
