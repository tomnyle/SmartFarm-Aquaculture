#include "ds18b20_driver.h"

DS18B20Driver::DS18B20Driver(uint8_t pin, bool enabled)
    : enabled_(enabled), oneWire_(pin), sensor_(&oneWire_) {}

bool DS18B20Driver::begin() {
  if (!enabled_) return false;
  sensor_.begin();
  return true;
}

SensorReading DS18B20Driver::read() {
  SensorReading reading{SensorKind::TEMPERATURE, "temperature", 0.0f, false, millis()};
  if (!enabled_) return reading;
  sensor_.requestTemperatures();
  const float value = sensor_.getTempCByIndex(0);
  reading.value = value;
  reading.available = value > -55.0f && value < 125.0f;
  return reading;
}
