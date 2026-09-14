#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <ArduinoJson.h>

// ==================== SENSOR CONFIGURATION ====================

struct SensorConfig {
    bool enabled;
    uint16_t read_interval;  // milliseconds
    float calibration_offset;
    float calibration_scale;
};

struct TemperatureSensorConfig {
    SensorConfig base;
    float min_alert;      // °C
    float max_alert;      // °C
};

struct PHSensorConfig {
    SensorConfig base;
    float calibration_point_1_voltage;
    float calibration_point_1_ph;
    float calibration_point_2_voltage;
    float calibration_point_2_ph;
};

struct DOSensorConfig {
    SensorConfig base;
    float calibration_voltage_0;    // 0% DO
    float calibration_voltage_100;  // 100% DO
};

struct WaterLevelConfig {
    SensorConfig base;
    float min_level;      // cm
    float max_level;      // cm
    float alert_level;    // cm
};

// ==================== OUTPUT CONFIGURATION ====================

struct RelayConfig {
    bool enabled;
    uint8_t pin;
    uint16_t min_on_time;   // milliseconds
    uint16_t min_off_time;  // milliseconds
    const char* name;
};

// ==================== DEVICE CONFIGURATION STRUCTURE ====================

struct DeviceConfig {
    char device_id[64];
    char device_name[64];
    char location[64];
    
    // Sensor configs
    TemperatureSensorConfig temperature;
    PHSensorConfig ph;
    DOSensorConfig dissolved_oxygen;
    WaterLevelConfig water_level;
    
    // Output configs
    RelayConfig relays[8];
    
    // System config
    char mode[16];  // AUTO, MANUAL, SCHEDULE, SAFE
    bool debug_enabled;
    uint16_t mqtt_update_interval;
    
    // Profiles
    char active_profile[32];
};

// ==================== LOAD / SAVE FUNCTIONS ====================

class ConfigManager {
public:
    static void loadDefaultConfig(DeviceConfig& config);
    static bool loadConfigFromFile(const char* path, DeviceConfig& config);
    static bool saveConfigToFile(const char* path, const DeviceConfig& config);
    static void printConfig(const DeviceConfig& config);
};

#endif // DEVICE_CONFIG_H
