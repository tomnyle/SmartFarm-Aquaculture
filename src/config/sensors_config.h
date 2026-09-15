#ifndef SMARTFARM_SENSORS_CONFIG_H
#define SMARTFARM_SENSORS_CONFIG_H

#include <Arduino.h>
#include "app_config.h"

// Compile-time feature flags / Cờ bật tắt cảm biến theo giai đoạn.
#define ENABLE_TEMPERATURE 1
#define ENABLE_PH 1
#define ENABLE_DO 1
#define ENABLE_WATER_LEVEL 1
#define ENABLE_EC_TDS 0
#define ENABLE_ORP 0
#define ENABLE_TURBIDITY 0
#define ENABLE_CO2 0
#define ENABLE_GAS_SENSOR 0

struct BaseSensorConfig {
    bool enabled;
    const char* id;
    float calibration_offset;
    float calibration_scale;
};

struct TemperatureSensorConfig {
    BaseSensorConfig base;
    uint8_t pin;
};

struct ADS1115SensorConfig {
    BaseSensorConfig base;
    uint8_t ads_index;
    uint8_t channel;
    float calibration_low_voltage;
    float calibration_low_value;
    float calibration_high_voltage;
    float calibration_high_value;
};

struct WaterLevelSensorConfig {
    BaseSensorConfig base;
    uint8_t pin;
    uint8_t active_state;
};

static const TemperatureSensorConfig TEMPERATURE_SENSOR_CONFIG = {
    {ENABLE_TEMPERATURE == 1, "temperature", 0.0F, 1.0F},
    DS18B20_PIN
};

static const ADS1115SensorConfig PH_SENSOR_CONFIG = {
    {ENABLE_PH == 1, "ph", 0.0F, 1.0F},
    1,
    0,
    2.50F,
    7.00F,
    3.00F,
    4.00F
};

static const ADS1115SensorConfig DO_SENSOR_CONFIG = {
    {ENABLE_DO == 1, "do", 0.0F, 1.0F},
    1,
    1,
    0.00F,
    0.00F,
    2.00F,
    20.00F
};

static const WaterLevelSensorConfig WATER_LEVEL_SENSOR_CONFIG = {
    {ENABLE_WATER_LEVEL == 1, "water_level", 0.0F, 1.0F},
    WATER_LEVEL_PIN,
    WATER_LEVEL_ACTIVE_STATE
};

static const ADS1115SensorConfig EC_TDS_SENSOR_CONFIG = {
    {ENABLE_EC_TDS == 1, "ec", 0.0F, 1.0F},
    2,
    0,
    0.0F,
    0.0F,
    1.0F,
    1.0F
};

static const ADS1115SensorConfig ORP_SENSOR_CONFIG = {
    {ENABLE_ORP == 1, "orp", 0.0F, 1.0F},
    2,
    1,
    0.0F,
    0.0F,
    1.0F,
    1.0F
};

static const ADS1115SensorConfig TURBIDITY_SENSOR_CONFIG = {
    {ENABLE_TURBIDITY == 1, "turbidity", 0.0F, 1.0F},
    2,
    2,
    0.0F,
    0.0F,
    1.0F,
    1.0F
};

static const ADS1115SensorConfig CO2_SENSOR_CONFIG = {
    {ENABLE_CO2 == 1, "co2", 0.0F, 1.0F},
    2,
    3,
    0.0F,
    0.0F,
    1.0F,
    1.0F
};

static const ADS1115SensorConfig GAS_SENSOR_CONFIG = {
    {ENABLE_GAS_SENSOR == 1, "gas_sensor", 0.0F, 1.0F},
    2,
    0,
    0.0F,
    0.0F,
    1.0F,
    1.0F
};

#endif
