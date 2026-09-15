#ifndef SRC_UTILS_DATA_TYPES_H
#define SRC_UTILS_DATA_TYPES_H

#include <Arduino.h>
#include "../../include/constants.h"

enum class ConditionState {
  NORMAL,
  WARNING,
  CRITICAL,
  UNKNOWN
};

enum class SensorKind {
  TEMPERATURE,
  PH,
  DISSOLVED_OXYGEN,
  WATER_LEVEL,
  EC_TDS,
  ORP,
  TURBIDITY,
  CO2,
  GAS_SENSOR
};

struct SensorReading {
  SensorKind kind;
  const char* key;
  float value;
  bool available;
  uint32_t updatedAt;
};

struct SpeciesRange {
  float min;
  float optimal;
  float max;
  float criticalMin;
  float criticalMax;
};

struct SpeciesProfile {
  const char* name;
  SpeciesRange temperature;
  SpeciesRange ph;
  struct {
    float min;
    float optimal;
    float criticalMin;
  } dissolvedOxygen;
  struct {
    float min;
    float optimal;
    float max;
    float criticalMin;
  } waterLevel;
};

struct RelaySnapshot {
  const char* name;
  bool state;
};

struct ScheduleEntry {
  char relayName[16];
  uint8_t hour;
  uint8_t minute;
  uint16_t durationSeconds;
  uint32_t lastRunDay;
};

struct ScheduleState {
  bool enabled;
  uint8_t entryCount;
  ScheduleEntry entries[constants::MAX_SCHEDULE_ENTRIES];
};

struct ConditionSummary {
  ConditionState temperature;
  ConditionState ph;
  ConditionState dissolvedOxygen;
  ConditionState waterLevel;
};

#endif
