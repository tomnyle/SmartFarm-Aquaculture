#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// ==================== SENSOR READINGS ====================

struct SensorReadings {
    float water_temperature;      // °C
    float water_ph;               // pH units
    float dissolved_oxygen;       // mg/L or %
    float water_level;            // cm
    
    uint32_t temperature_timestamp;
    uint32_t ph_timestamp;
    uint32_t do_timestamp;
    uint32_t level_timestamp;
    
    bool temperature_valid;
    bool ph_valid;
    bool do_valid;
    bool level_valid;
};

// ==================== OUTPUT STATES ====================

struct OutputStates {
    bool aerator;           // Máy sục khí
    bool water_pump;        // Bơm cấp nước
    bool circulation;       // Bơm tuần hoàn
    bool feeder;            // Máy cho ăn
    bool valve;             // Van
    bool light;             // Đèn
    bool spare1;            // Dự phòng 1
    bool spare2;            // Dự phòng 2
    
    uint32_t aerator_on_time;
    uint32_t water_pump_on_time;
    uint32_t circulation_on_time;
    uint32_t feeder_on_time;
};

// ==================== SYSTEM STATUS ====================

enum SystemMode {
    MODE_AUTO,      // Tự động theo cảm biến
    MODE_MANUAL,    // Điều khiển từ HA
    MODE_SCHEDULE,  // Lịch biểu
    MODE_SAFE       // Chế độ an toàn
};

enum SystemStatus {
    STATUS_INITIALIZING,
    STATUS_RUNNING,
    STATUS_ERROR,
    STATUS_SAFE,
    STATUS_DISCONNECTED
};

enum ErrorCode {
    ERROR_NONE = 0,
    ERROR_TEMP_SENSOR = 1,
    ERROR_PH_SENSOR = 2,
    ERROR_DO_SENSOR = 3,
    ERROR_LEVEL_SENSOR = 4,
    ERROR_MQTT_DISCONNECTED = 5,
    ERROR_WATER_LEVEL_LOW = 6,
    ERROR_TEMP_CRITICAL = 7,
    ERROR_DO_CRITICAL = 8,
    ERROR_WATCHDOG = 9
};

struct SystemState {
    SystemMode mode;
    SystemStatus status;
    uint32_t status_timestamp;
    
    uint32_t uptime;           // seconds
    uint32_t last_mqtt_message; // timestamp
    uint32_t last_rule_run;     // timestamp
    
    float cpu_load;            // 0.0 - 100.0
    uint32_t free_memory;
    
    bool mqtt_connected;
    bool ha_discovered;        // Home Assistant discovered
    bool config_valid;
    
    uint16_t error_code;       // ErrorCode
    char error_message[128];
};

// ==================== COMPLETE SYSTEM STATE ====================

struct AquacultureSystemState {
    // Device info
    char device_id[64];
    char firmware_version[32];
    uint32_t startup_time;
    
    // Data
    SensorReadings sensors;
    OutputStates outputs;
    SystemState system;
    
    // Active profile
    char active_profile[32];
    
    // Last 10 events (for debugging)
    struct {
        uint32_t timestamp;
        char message[128];
    } events[10];
    uint8_t event_index;
};

// ==================== STATE MANAGEMENT ====================

class StateManager {
public:
    static void initializeState(AquacultureSystemState& state);
    static void updateSensorReading(AquacultureSystemState& state, const char* sensor_type, float value);
    static void updateOutputState(AquacultureSystemState& state, const char* output_name, bool state);
    static void setSystemMode(AquacultureSystemState& state, SystemMode mode);
    static void setSystemStatus(AquacultureSystemState& state, SystemStatus status, ErrorCode error = ERROR_NONE);
    static void addEvent(AquacultureSystemState& state, const char* message);
    static JsonDocument stateToJson(const AquacultureSystemState& state);
    static void printState(const AquacultureSystemState& state);
};

#endif // SYSTEM_STATE_H
