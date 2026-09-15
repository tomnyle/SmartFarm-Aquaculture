#ifndef PINS_H
#define PINS_H

// ==================== SENSOR PINS ====================
// Analog Input Pins
#define PH_PIN 34           // ADC1_CH6 - pH sensor
#define TURBIDITY_PIN 35    // ADC1_CH7 - Turbidity sensor
#define DO_PIN 36           // ADC1_CH0 - Dissolved Oxygen sensor
#define CO2_PIN 39          // ADC1_CH3 - CO2 sensor

// 1-Wire Bus
#define ONE_WIRE_BUS 4      // DS18B20 temperature sensor

// DHT Sensor
#define DHTPIN 15           // DHT22 data pin
#define DHTTYPE DHT22       // DHT22 sensor type

// I2C Pins (BH1750 Light Sensor)
#define I2C_SDA 21          // I2C Data
#define I2C_SCL 22          // I2C Clock

// ==================== OUTPUT PINS ====================
// Relay Pins
#define PUMP_PIN 13         // Main pump relay
#define AERATOR_PIN 12      // Aerator relay
#define CIRCULATION_PIN 14  // Circulation pump relay
#define FEEDER_PIN 27       // Automatic feeder relay

// Status LED (Optional)
#define LED_PIN 2           // Built-in LED for status

// ==================== SPI PINS (Optional - Reserved) ====================
// #define SPI_MOSI 23
// #define SPI_MISO 19
// #define SPI_CLK 18

#endif // PINS_H
