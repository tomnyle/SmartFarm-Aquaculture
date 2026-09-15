#include "system_state.h"

void initializeSystemState(SystemState& state, const char* profileName) {
  state.mode = OperationMode::AUTO;
  state.selectedMode = OperationMode::AUTO;
  state.activeProfile = profileName;
  state.status = "INIT";
  state.error = "";
  state.conditions = {ConditionState::UNKNOWN, ConditionState::UNKNOWN, ConditionState::UNKNOWN, ConditionState::UNKNOWN};
  state.schedule = {};
  state.startedAt = millis();
  state.lastSensorPoll = 0;
  state.lastMqttPublish = 0;
  state.lastRuleEvaluation = 0;
}
