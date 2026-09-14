#ifndef PINS_H
#define PINS_H

// ==================== SENSOR PINS ====================

// Temperature Sensor (DS18B20 - 1-Wire)
#define PIN_TEMPERATURE_SENSOR 4  // GPIO4

// pH Sensor (Analog via ADS1115)
#define ADS1115_ADDRESS 0x48
#define ADS1115_CHANNEL_PH 0  // A0

// Dissolved Oxygen Sensor (Analog via ADS1115)
#define ADS1115_CHANNEL_DO 1   // A1

// Water Level Sensor (Analog or Digital)
#define PIN_WATER_LEVEL 34  // GPIO34 (ADC1_CH6) - Input only

// ==================== OUTPUT PINS (RELAY) ====================

// Relay Module (8-channel)
#define PIN_RELAY_AERATOR 32       // GPIO32 - Máy sục khí
#define PIN_RELAY_WATER_PUMP 33    // GPIO33 - Bơm cấp nước
#define PIN_RELAY_CIRCULATION 25   // GPIO25 - Bơm tuần hoàn
#define PIN_RELAY_FEEDER 26        // GPIO26 - Máy cho ăn
#define PIN_RELAY_VALVE 27         // GPIO27 - Van
#define PIN_RELAY_LIGHT 14         // GPIO14 - Đèn
#define PIN_RELAY_SPARE1 12        // GPIO12 - Dự phòng 1
#define PIN_RELAY_SPARE2 13        // GPIO13 - Dự phòng 2

// ==================== COMMUNICATION PINS ====================

// UART for RS485 (if needed in future)
#define PIN_RS485_RX 16      // GPIO16
#define PIN_RS485_TX 17      // GPIO17
#define PIN_RS485_RE 5       // GPIO5 (Receive Enable)
#define PIN_RS485_DE 19      // GPIO19 (Driver Enable)

// I2C for ADS1115
#define PIN_I2C_SDA 21       // GPIO21
#define PIN_I2C_SCL 22       // GPIO22

// ==================== STATUS INDICATORS ====================

#define PIN_LED_POWER 2      // GPIO2 (Built-in LED)
#define PIN_LED_MQTT 15      // GPIO15
#define PIN_BUZZER 23        // GPIO23

// ==================== BUTTON/RESET ====================

#define PIN_RESET_BUTTON 0   // GPIO0 (Boot button - use with caution)

// ==================== RESERVED PINS ====================
// GPIO 6-11: Connected to integrated SPI flash
// GPIO 20: NC
// GPIO 24: NC
// GPIO 28-31: NC

// ==================== PIN CONFIGURATION ====================

// Relay modes
#define RELAY_MODE_ACTIVE_LOW true   // true = LOW turns ON, false = HIGH turns ON

// ADC Configuration
#define ADC_RESOLUTION 12  // bits
#define ADC_ATTENUATION ADC_11db  // 0-3.6V range

#endif // PINS_H
