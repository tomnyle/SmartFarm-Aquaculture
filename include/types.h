#ifndef SMARTFARM_TYPES_H
#define SMARTFARM_TYPES_H

#include <Arduino.h>

// Common runtime status / Trạng thái chung của cảm biến.
enum SensorStatus {
    SENSOR_STATUS_DISABLED = 0,
    SENSOR_STATUS_OK = 1,
    SENSOR_STATUS_ERROR = 2
};

// Relay state model / Mô hình trạng thái relay.
enum RelayState {
    RELAY_OFF = 0,
    RELAY_ON = 1,
    RELAY_ERROR = 2,
    RELAY_AUTO = 3
};

// System lifecycle state / Trạng thái vòng đời hệ thống.
enum SystemState {
    SYSTEM_INIT = 0,
    SYSTEM_READY = 1,
    SYSTEM_RUNNING = 2,
    SYSTEM_ERROR = 3,
    SYSTEM_SAFE = 4
};

// Shared sensor reading structure / Cấu trúc dữ liệu đọc cảm biến dùng chung.
struct SensorReading {
    uint32_t timestamp;
    float value;
    SensorStatus status;
    uint16_t error_count;
};

// Aggregated sensor snapshot / Ảnh chụp nhanh toàn bộ cảm biến.
struct SensorSnapshot {
    SensorReading temperature;
    SensorReading ph;
    SensorReading dissolved_oxygen;
    SensorReading water_level;
    SensorReading ec;
    SensorReading orp;
    SensorReading turbidity;
    SensorReading co2;
    SensorReading gas_sensor;
};

// Relay runtime entry / Trạng thái runtime của từng relay.
struct RelayChannelState {
    const char* name;
    RelayState state;
    bool enabled;
};

// Species profile used by rule engine / Hồ sơ đối tượng nuôi dùng cho rule engine.
struct SpeciesProfile {
    const char* id;
    const char* display_name;
    float temperature_min;
    float temperature_max;
    float ph_min;
    float ph_max;
    float dissolved_oxygen_min;
    bool low_level_triggers_safe;
};

#endif
