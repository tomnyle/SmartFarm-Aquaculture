#ifndef SRC_UTILS_HELPERS_H
#define SRC_UTILS_HELPERS_H

#include <Arduino.h>
#include "data_types.h"

namespace helpers {
inline float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

inline const char* conditionToString(ConditionState state) {
  switch (state) {
    case ConditionState::NORMAL: return "NORMAL";
    case ConditionState::WARNING: return "WARNING";
    case ConditionState::CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

inline bool equalsIgnoreCase(const String& left, const char* right) {
  return left.equalsIgnoreCase(right);
}
}

#endif
