#ifndef SRC_CONFIG_SENSORS_CONFIG_H
#define SRC_CONFIG_SENSORS_CONFIG_H

#include <stdint.h>

namespace sensors_config {
constexpr bool ENABLE_DS18B20 = true;
constexpr bool ENABLE_PH = true;
constexpr bool ENABLE_DO = true;
constexpr bool ENABLE_WATER_LEVEL = true;

constexpr bool ENABLE_EC_TDS = false;
constexpr bool ENABLE_ORP = false;
constexpr bool ENABLE_TURBIDITY = false;

constexpr bool ENABLE_CO2 = false;
constexpr bool ENABLE_GAS_SENSOR = false;

constexpr uint8_t ONE_WIRE_PIN = 4;
constexpr uint8_t I2C_SDA_PIN = 21;
constexpr uint8_t I2C_SCL_PIN = 22;
constexpr uint8_t PH_ADC_CHANNEL = 0;
constexpr uint8_t DO_ADC_CHANNEL = 1;
constexpr uint8_t WATER_LEVEL_PIN = 34;
constexpr uint8_t EC_ADC_CHANNEL = 2;
constexpr uint8_t ORP_ADC_CHANNEL = 3;
}

#endif
