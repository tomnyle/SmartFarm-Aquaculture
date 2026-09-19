#ifndef AQUACULTURE_LOGIC_H
#define AQUACULTURE_LOGIC_H

#include <Arduino.h>
#include <string.h>

enum class ControlMode { AUTO, MANUAL, SCHEDULE, SAFE, EMERGENCY };
enum class SafetyLevel { NORMAL, WARNING, LOW, CRITICAL, EMERGENCY, SENSOR_FAULT };

struct SensorData {
  float water_temp;
  float air_temp;
  float air_humidity;
  float ph;
  float ph_rate;
  float turbidity;
  float do_value;
  float co2;
  float light;
  float water_level;
  float aerator_current;
  float pump_current;

  uint32_t last_read;
  uint32_t water_temp_last_valid;
  uint32_t air_last_valid;
  uint32_t ph_last_valid;
  uint32_t do_last_valid;
  uint32_t water_level_last_valid;
  uint32_t aerator_current_last_valid;
  uint32_t pump_current_last_valid;

  bool water_temp_valid;
  bool air_valid;
  bool ph_valid;
  bool do_valid;
  bool turbidity_valid;
  bool co2_valid;
  bool light_valid;
  bool water_level_valid;
  bool aerator_current_valid;
  bool pump_current_valid;

  bool water_temp_stale;
  bool air_stale;
  bool ph_stale;
  bool do_stale;
  bool water_level_stale;
  bool aerator_current_stale;
  bool pump_current_stale;
};

struct OutputState {
  bool pump;
  bool aerator1;
  bool aerator2;
  bool circulation;
  bool feeder;
  bool alarm;
  bool feeder_locked;
};

struct ControlState {
  ControlMode requested_mode;
  ControlMode effective_mode;
  SafetyLevel safety_level;
  bool alarm_active;
  bool emergency_active;
  bool safety_active;
  bool requires_ack;
  uint32_t recovery_since;
  uint32_t manual_override_since;
  uint32_t pump_fault_since;
  uint32_t aerator_fault_since;
  char alarm_text[160];
  char event_key[64];
};

struct RelayTracker {
  uint32_t last_change;
  uint32_t on_since;
};

inline const char* controlModeToString(ControlMode mode) {
  switch (mode) {
    case ControlMode::AUTO: return "AUTO";
    case ControlMode::MANUAL: return "MANUAL";
    case ControlMode::SCHEDULE: return "SCHEDULE";
    case ControlMode::SAFE: return "SAFE";
    case ControlMode::EMERGENCY: return "EMERGENCY";
  }
  return "AUTO";
}

inline const char* safetyLevelToString(SafetyLevel level) {
  switch (level) {
    case SafetyLevel::NORMAL: return "NORMAL";
    case SafetyLevel::WARNING: return "WARNING";
    case SafetyLevel::LOW: return "LOW";
    case SafetyLevel::CRITICAL: return "CRITICAL";
    case SafetyLevel::EMERGENCY: return "EMERGENCY";
    case SafetyLevel::SENSOR_FAULT: return "SENSOR_FAULT";
  }
  return "NORMAL";
}

inline ControlMode parseControlMode(const char* mode) {
  if (strcmp(mode, "MANUAL") == 0) return ControlMode::MANUAL;
  if (strcmp(mode, "SCHEDULE") == 0) return ControlMode::SCHEDULE;
  if (strcmp(mode, "SAFE") == 0) return ControlMode::SAFE;
  if (strcmp(mode, "EMERGENCY") == 0) return ControlMode::EMERGENCY;
  return ControlMode::AUTO;
}

inline void clearOutputState(OutputState& state) {
  state.pump = false;
  state.aerator1 = false;
  state.aerator2 = false;
  state.circulation = false;
  state.feeder = false;
  state.alarm = false;
  state.feeder_locked = false;
}

inline void initializeControlState(ControlState& state, const char* default_mode) {
  state.requested_mode = parseControlMode(default_mode);
  state.effective_mode = state.requested_mode;
  state.safety_level = SafetyLevel::NORMAL;
  state.alarm_active = false;
  state.emergency_active = false;
  state.safety_active = false;
  state.requires_ack = false;
  state.recovery_since = 0;
  state.manual_override_since = 0;
  state.pump_fault_since = 0;
  state.aerator_fault_since = 0;
  state.alarm_text[0] = '\0';
  state.event_key[0] = '\0';
}

#endif // AQUACULTURE_LOGIC_H
