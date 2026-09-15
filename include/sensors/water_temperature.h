#ifndef WATER_TEMPERATURE_H
#define WATER_TEMPERATURE_H

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

class WaterTemperatureSensor {
public:
    WaterTemperatureSensor(uint8_t pin);
    
    bool initialize();
    void update();
    
    float getTemperature() const { return last_temperature; }
    bool isValid() const { return is_valid; }
    uint32_t getLastReadTime() const { return last_read_time; }
    
    void setCalibrationOffset(float offset) { calibration_offset = offset; }
    void setCalibrationScale(float scale) { calibration_scale = scale; }
    
    float getRawTemperature() const { return raw_temperature; }
    
private:
    uint8_t sensor_pin;
    OneWire one_wire;
    DallasTemperature ds_sensor;
    
    float last_temperature;
    float raw_temperature;
    float calibration_offset;
    float calibration_scale;
    
    bool is_valid;
    uint32_t last_read_time;
    uint32_t last_update_time;
    
    static const uint32_t READ_INTERVAL = 30000;  // 30 seconds
    static const uint8_t RESOLUTION = 12;  // bits
};

#endif // WATER_TEMPERATURE_H
