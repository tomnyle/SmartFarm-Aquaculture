#include "do_sensor_driver.h"

DOSensorDriver::DOSensorDriver(Adafruit_ADS1115* ads, const ADS1115SensorConfig& config)
    : ads_(ads), config_(config), error_count_(0) {}

bool DOSensorDriver::begin() {
    return config_.base.enabled && ads_ != nullptr;
}

SensorReading DOSensorDriver::read() {
    SensorReading reading = {millis(), 0.0F, SENSOR_STATUS_DISABLED, error_count_};
    if (!config_.base.enabled || ads_ == nullptr) {
        return reading;
    }

    const int16_t raw = ads_->readADC_SingleEnded(config_.channel);
    const float voltage = toVoltage(raw);
    reading.value = Calibration::linearMap(
        voltage,
        config_.calibration_low_voltage,
        config_.calibration_high_voltage,
        config_.calibration_low_value,
        config_.calibration_high_value
    );
    reading.value = (reading.value + config_.base.calibration_offset) * config_.base.calibration_scale;
    reading.status = SENSOR_STATUS_OK;
    reading.error_count = error_count_;
    return reading;
}

bool DOSensorDriver::isEnabled() const {
    return config_.base.enabled;
}

const char* DOSensorDriver::getId() const {
    return config_.base.id;
}

uint8_t DOSensorDriver::getBusIndex() const {
    return config_.ads_index;
}

float DOSensorDriver::toVoltage(int16_t raw) const {
    return raw * 0.1875F / 1000.0F;
}
