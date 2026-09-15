#ifndef SRC_SYSTEM_SYSTEM_STATE_H
#define SRC_SYSTEM_SYSTEM_STATE_H

#include <Arduino.h>
#include "../modes/operation_mode.h"
#include "../utils/data_types.h"

struct SystemState {
  OperationMode mode;
  OperationMode selectedMode;
  String activeProfile;
  String status;
  String error;
  ConditionSummary conditions;
  ScheduleState schedule;
  uint32_t startedAt;
  uint32_t lastSensorPoll;
  uint32_t lastMqttPublish;
  uint32_t lastRuleEvaluation;
};

void initializeSystemState(SystemState& state, const char* profileName);

#endif
