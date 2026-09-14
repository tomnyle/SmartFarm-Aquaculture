#ifndef DO_SENSOR_H
#define DO_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

class DissolvedOxygenSensor {
public:
    DissolvedOxygenSensor(uint8_t i2c_sda, uint8_t i2c_scl, uint8_t ads_address = 0x48);
    
    bool initialize();
    void update();
    
    float getDO() const { return last_do; }           // mg/L
    float getDOPercent() const { return last_do_percent; }  // %
    float getVoltage() const { return last_voltage; }
    bool isValid() const { return is_valid; }
    uint32_t getLastReadTime() const { return last_read_time; }
    
    // Two-point calibration
    void calibrate(float voltage_0do, float voltage_100do);
    void setCalibrationPoints(float v_0, float v_100);
    
    float getRawADC() const { return raw_adc; }
    
    // Temperature compensation
    void setWaterTemperature(float temp) { water_temperature = temp; }
    
private:
    Adafruit_ADS1115 ads;
    uint8_t ads_address;
    
    float last_do;           // mg/L
    float last_do_percent;   // %
    float last_voltage;
    float raw_adc;
    
    // Calibration points
    float cal_voltage_0do;   // Voltage at 0% DO
    float cal_voltage_100do; // Voltage at 100% DO
    
    float water_temperature; // for compensation
    
    bool is_calibrated;
    bool is_valid;
    uint32_t last_read_time;
    uint32_t last_update_time;
    
    float voltageToMV(int16_t adc_value) const;
    float calculateDO(float voltage_mv);
    float compensateTemperature(float do_value);
    
    static const uint32_t READ_INTERVAL = 30000;  // 30 seconds
    static const float ADC_SCALE;
    static const float DO_AT_SEALEVEL_MGL;  // ~8.6 mg/L
};

#endif // DO_SENSOR_H
