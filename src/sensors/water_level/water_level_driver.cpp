#include <Arduino.h>
#include "water_level_driver.h"

WaterLevelDriver::WaterLevelDriver(const WaterLevelSensorConfig& config)
    : config_(config), error_count_(0) {}

bool WaterLevelDriver::begin() {
    if (!config_.base.enabled) {
        return false;
    }

    pinMode(config_.pin, INPUT_PULLUP);
    return true;
}

SensorReading WaterLevelDriver::read() {
    SensorReading reading = {millis(), 0.0F, SENSOR_STATUS_DISABLED, error_count_};
    if (!config_.base.enabled) {
        return reading;
    }

    reading.value = digitalRead(config_.pin) == config_.active_state ? 0.0F : 1.0F;
    reading.status = SENSOR_STATUS_OK;
    reading.error_count = error_count_;
    return reading;
}

bool WaterLevelDriver::isEnabled() const {
    return config_.base.enabled;
}

const char* WaterLevelDriver::getId() const {
    return config_.base.id;
}
