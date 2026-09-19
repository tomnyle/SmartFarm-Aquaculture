#ifndef PINS_H
#define PINS_H

// ==================== SENSOR PINS ====================
// Analog Input Pins (ADC1 where possible for WiFi-safe reads)
#define PH_PIN 34                 // ADC1_CH6 - pH sensor input
#define TURBIDITY_PIN 35          // ADC1_CH7 - turbidity sensor input
#define DO_PIN 36                 // ADC1_CH0 - dissolved oxygen sensor input
#define CO2_PIN 39                // ADC1_CH3 - CO2 sensor input
#define WATER_LEVEL_PIN 32        // ADC1_CH4 - water level sensor input
#define AERATOR_CURRENT_PIN 33    // ADC1_CH5 - aerator current sensor input
#define PUMP_CURRENT_PIN 25       // ADC2_CH8 - optional pump current hook; prefer external ADC in WiFi-heavy installs

// 1-Wire Bus
#define ONE_WIRE_BUS 4            // DS18B20 water temperature sensor

// DHT Sensor
#define DHTPIN 15                 // DHT22 data pin
#define DHTTYPE DHT22             // DHT22 sensor type

// I2C Pins (BH1750 / optional ADS1115 expansion)
#define I2C_SDA 21                // I2C data
#define I2C_SCL 22                // I2C clock

// ==================== OUTPUT PINS ====================
// Relay / actuator outputs
#define PUMP_PIN 13               // Main water pump relay output
#define AERATOR_1_PIN 12          // Aerator 1 relay output
#define CIRCULATION_PIN 14        // Circulation pump relay output
#define FEEDER_PIN 27             // Automatic feeder relay output
#define AERATOR_2_PIN 26          // Aerator 2 relay output
#define ALARM_PIN 23              // Alarm buzzer / beacon relay output

// Backward-compatible aliases
#define AERATOR_PIN AERATOR_1_PIN

// Status LED (Optional)
#define LED_PIN 2                 // Built-in status LED output

#endif // PINS_H
