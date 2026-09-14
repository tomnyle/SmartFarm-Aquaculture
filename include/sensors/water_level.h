#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <Arduino.h>

class WaterLevelSensor {
public:
    WaterLevelSensor(uint8_t pin);
    
    bool initialize();
    void update();
    
    float getLevel() const { return last_level; }  // cm
    float getLevelPercent() const { return level_percent; }  // 0-100%
    bool isValid() const { return is_valid; }
    uint32_t getLastReadTime() const { return last_read_time; }
    
    // Calibration
    void setMinMaxLevel(float min_cm, float max_cm);
    void setCalibrationPoints(float adc_min, float adc_max, float level_min, float level_max);
    
    float getRawADC() const { return raw_adc; }
    
private:
    uint8_t sensor_pin;
    
    float last_level;      // cm
    float level_percent;   // 0-100%
    float raw_adc;
    
    // Calibration
    float adc_value_at_min;
    float adc_value_at_max;
    float level_at_min;    // cm
    float level_at_max;    // cm
    
    bool is_calibrated;
    bool is_valid;
    uint32_t last_read_time;
    uint32_t last_update_time;
    
    float adcToLevel(float adc_value);
    float readADC();
    
    static const uint32_t READ_INTERVAL = 30000;  // 30 seconds
    static const uint16_t ADC_RESOLUTION = 4095;  // 12-bit
    static const uint8_t FILTER_SAMPLES = 10;
};

#endif // WATER_LEVEL_H
