#ifndef SRC_RULES_CONDITIONS_H
#define SRC_RULES_CONDITIONS_H

#include "../utils/data_types.h"

struct RuleEvaluation {
  ConditionSummary conditions;
  bool aeratorOn;
  bool pumpOn;
  bool circulationOn;
  bool safeModeRequired;
  const char* reason;
};

#endif
