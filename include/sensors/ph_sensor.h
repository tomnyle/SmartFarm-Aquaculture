#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

class PHSensor {
public:
    PHSensor(uint8_t i2c_sda, uint8_t i2c_scl, uint8_t ads_address = 0x48);
    
    bool initialize();
    void update();
    
    float getPH() const { return last_ph; }
    float getVoltage() const { return last_voltage; }
    bool isValid() const { return is_valid; }
    uint32_t getLastReadTime() const { return last_read_time; }
    
    // Two-point calibration
    void calibrate(float voltage1, float ph1, float voltage2, float ph2);
    void setCalibrationPoint1(float voltage, float ph);
    void setCalibrationPoint2(float voltage, float ph);
    
    float getRawADC() const { return raw_adc; }
    
private:
    Adafruit_ADS1115 ads;
    uint8_t ads_address;
    
    float last_ph;
    float last_voltage;
    float raw_adc;
    
    // Calibration points
    float cal_voltage_1;
    float cal_ph_1;
    float cal_voltage_2;
    float cal_ph_2;
    
    bool is_calibrated;
    bool is_valid;
    uint32_t last_read_time;
    uint32_t last_update_time;
    
    float voltageToMV(int16_t adc_value) const;
    float calculatePH(float voltage_mv);
    
    static const uint32_t READ_INTERVAL = 30000;  // 30 seconds
    static const float ADC_SCALE;  // Scale factor for conversion
};

#endif // PH_SENSOR_H
