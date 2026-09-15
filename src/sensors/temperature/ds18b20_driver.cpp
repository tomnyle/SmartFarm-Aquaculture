#include "ds18b20_driver.h"

DS18B20Driver::DS18B20Driver(const TemperatureSensorConfig& config)
    : config_(config), one_wire_(config.pin), sensor_bus_(&one_wire_), error_count_(0) {}

bool DS18B20Driver::begin() {
    if (!config_.base.enabled) {
        return false;
    }

    sensor_bus_.begin();
    sensor_bus_.setResolution(12);
    return true;
}

SensorReading DS18B20Driver::read() {
    SensorReading reading = {millis(), 0.0F, SENSOR_STATUS_DISABLED, error_count_};
    if (!config_.base.enabled) {
        return reading;
    }

    sensor_bus_.requestTemperatures();
    const float raw = sensor_bus_.getTempCByIndex(0);
    if (raw <= -127.0F) {
        error_count_++;
        reading.status = SENSOR_STATUS_ERROR;
        reading.error_count = error_count_;
        return reading;
    }

    reading.value = (raw + config_.base.calibration_offset) * config_.base.calibration_scale;
    reading.status = SENSOR_STATUS_OK;
    reading.error_count = error_count_;
    return reading;
}

bool DS18B20Driver::isEnabled() const {
    return config_.base.enabled;
}

const char* DS18B20Driver::getId() const {
    return config_.base.id;
}
