#ifndef OPERATION_PROFILE_H
#define OPERATION_PROFILE_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#endif

enum OperationProfileId {
  PROFILE_SENSOR_TEST = 0,
  PROFILE_NO_LIVESTOCK_TEST = 1,
  PROFILE_PRODUCTION = 2
};

enum RelayTestStatusId {
  RELAY_TEST_NOT_STARTED = 0,
  RELAY_TEST_IN_PROGRESS = 1,
  RELAY_TEST_PASSED = 2,
  RELAY_TEST_FAILED = 3
};

enum ProfileBlockReason : uint16_t {
  BLOCK_REQUIRED_SENSOR_INVALID = 1 << 0,
  BLOCK_SENSOR_FAULT_ACTIVE = 1 << 1,
  BLOCK_RELAY_TEST_NOT_PASSED = 1 << 2,
  BLOCK_OUTPUTS_LOCKED = 1 << 3,
  BLOCK_SENSOR_TEST_MODE_ENABLED = 1 << 4,
  BLOCK_ACTIVE_ALARM = 1 << 5,
  BLOCK_LIVESTOCK_NOT_CONFIRMED = 1 << 6,
  BLOCK_CRITICAL_CONDITION_ACTIVE = 1 << 7
};

struct ProfileEligibilityInputs {
  bool required_sensors_valid;
  bool sensor_fault_active;
  bool relay_test_passed;
  bool outputs_locked;
  bool sensor_test_mode_enabled;
  bool active_alarm;
  bool critical_condition_active;
  bool livestock_present_confirmed;
};

struct ProfileEvaluation {
  OperationProfileId requested_profile;
  OperationProfileId actual_profile;
  bool can_no_load_test;
  bool can_production;
  uint16_t no_load_blockers;
  uint16_t production_blockers;
  uint16_t requested_blockers;
};

inline const char* operationProfileToString(OperationProfileId profile) {
  switch (profile) {
    case PROFILE_NO_LIVESTOCK_TEST:
      return "NO_LIVESTOCK_TEST";
    case PROFILE_PRODUCTION:
      return "PRODUCTION";
    case PROFILE_SENSOR_TEST:
    default:
      return "SENSOR_TEST";
  }
}

inline OperationProfileId operationProfileFromString(const char* value) {
  if (strcmp(value, "NO_LIVESTOCK_TEST") == 0) {
    return PROFILE_NO_LIVESTOCK_TEST;
  }
  if (strcmp(value, "PRODUCTION") == 0) {
    return PROFILE_PRODUCTION;
  }
  return PROFILE_SENSOR_TEST;
}

inline const char* relayTestStatusToString(RelayTestStatusId status) {
  switch (status) {
    case RELAY_TEST_IN_PROGRESS:
      return "IN_PROGRESS";
    case RELAY_TEST_PASSED:
      return "PASSED";
    case RELAY_TEST_FAILED:
      return "FAILED";
    case RELAY_TEST_NOT_STARTED:
    default:
      return "NOT_STARTED";
  }
}

inline RelayTestStatusId relayTestStatusFromString(const char* value) {
  if (strcmp(value, "IN_PROGRESS") == 0) {
    return RELAY_TEST_IN_PROGRESS;
  }
  if (strcmp(value, "PASSED") == 0) {
    return RELAY_TEST_PASSED;
  }
  if (strcmp(value, "FAILED") == 0) {
    return RELAY_TEST_FAILED;
  }
  return RELAY_TEST_NOT_STARTED;
}

inline uint16_t computeNoLoadBlockers(const ProfileEligibilityInputs& inputs) {
  uint16_t blockers = 0;

  if (!inputs.required_sensors_valid) {
    blockers |= BLOCK_REQUIRED_SENSOR_INVALID;
  }
  if (inputs.sensor_fault_active) {
    blockers |= BLOCK_SENSOR_FAULT_ACTIVE;
  }
  if (!inputs.relay_test_passed) {
    blockers |= BLOCK_RELAY_TEST_NOT_PASSED;
  }
  if (inputs.outputs_locked) {
    blockers |= BLOCK_OUTPUTS_LOCKED;
  }
  if (inputs.sensor_test_mode_enabled) {
    blockers |= BLOCK_SENSOR_TEST_MODE_ENABLED;
  }
  if (inputs.critical_condition_active) {
    blockers |= BLOCK_CRITICAL_CONDITION_ACTIVE;
  }

  return blockers;
}

inline uint16_t computeProductionBlockers(const ProfileEligibilityInputs& inputs) {
  uint16_t blockers = computeNoLoadBlockers(inputs);

  if (inputs.active_alarm) {
    blockers |= BLOCK_ACTIVE_ALARM;
  }
  if (!inputs.livestock_present_confirmed) {
    blockers |= BLOCK_LIVESTOCK_NOT_CONFIRMED;
  }

  return blockers;
}

inline ProfileEvaluation evaluateProfileRequest(OperationProfileId requested_profile,
                                                const ProfileEligibilityInputs& inputs) {
  ProfileEvaluation result;
  result.requested_profile = requested_profile;
  result.no_load_blockers = computeNoLoadBlockers(inputs);
  result.production_blockers = computeProductionBlockers(inputs);
  result.can_no_load_test = result.no_load_blockers == 0;
  result.can_production = result.production_blockers == 0;
  result.requested_blockers = 0;
  result.actual_profile = PROFILE_SENSOR_TEST;

  switch (requested_profile) {
    case PROFILE_NO_LIVESTOCK_TEST:
      result.requested_blockers = result.no_load_blockers;
      result.actual_profile = result.can_no_load_test ? PROFILE_NO_LIVESTOCK_TEST : PROFILE_SENSOR_TEST;
      break;
    case PROFILE_PRODUCTION:
      result.requested_blockers = result.production_blockers;
      if (result.can_production) {
        result.actual_profile = PROFILE_PRODUCTION;
      } else if (result.can_no_load_test) {
        result.actual_profile = PROFILE_NO_LIVESTOCK_TEST;
      } else {
        result.actual_profile = PROFILE_SENSOR_TEST;
      }
      break;
    case PROFILE_SENSOR_TEST:
    default:
      result.requested_blockers = 0;
      result.actual_profile = PROFILE_SENSOR_TEST;
      break;
  }

  return result;
}

inline const char* blockReasonCode(uint16_t reason_bit) {
  switch (reason_bit) {
    case BLOCK_REQUIRED_SENSOR_INVALID:
      return "required_sensor_invalid";
    case BLOCK_SENSOR_FAULT_ACTIVE:
      return "sensor_fault_active";
    case BLOCK_RELAY_TEST_NOT_PASSED:
      return "relay_test_not_passed";
    case BLOCK_OUTPUTS_LOCKED:
      return "outputs_locked";
    case BLOCK_SENSOR_TEST_MODE_ENABLED:
      return "sensor_test_mode_enabled";
    case BLOCK_ACTIVE_ALARM:
      return "active_alarm";
    case BLOCK_LIVESTOCK_NOT_CONFIRMED:
      return "livestock_not_confirmed";
    case BLOCK_CRITICAL_CONDITION_ACTIVE:
      return "critical_condition_active";
    default:
      return "unknown";
  }
}

inline const char* blockReasonText(uint16_t reason_bit) {
  switch (reason_bit) {
    case BLOCK_REQUIRED_SENSOR_INVALID:
      return "Required commissioning sensors are not valid";
    case BLOCK_SENSOR_FAULT_ACTIVE:
      return "A required sensor fault is active";
    case BLOCK_RELAY_TEST_NOT_PASSED:
      return "Relay test status is not PASSED";
    case BLOCK_OUTPUTS_LOCKED:
      return "Outputs are locked";
    case BLOCK_SENSOR_TEST_MODE_ENABLED:
      return "SENSOR_TEST_MODE is enabled";
    case BLOCK_ACTIVE_ALARM:
      return "An active alarm is present";
    case BLOCK_LIVESTOCK_NOT_CONFIRMED:
      return "Livestock presence is not confirmed";
    case BLOCK_CRITICAL_CONDITION_ACTIVE:
      return "A critical condition is active";
    default:
      return "Unknown blocker";
  }
}

inline const uint16_t* orderedBlockReasons(size_t& count) {
  static const uint16_t reasons[] = {
    BLOCK_REQUIRED_SENSOR_INVALID,
    BLOCK_SENSOR_FAULT_ACTIVE,
    BLOCK_RELAY_TEST_NOT_PASSED,
    BLOCK_OUTPUTS_LOCKED,
    BLOCK_SENSOR_TEST_MODE_ENABLED,
    BLOCK_ACTIVE_ALARM,
    BLOCK_LIVESTOCK_NOT_CONFIRMED,
    BLOCK_CRITICAL_CONDITION_ACTIVE
  };
  count = sizeof(reasons) / sizeof(reasons[0]);
  return reasons;
}

inline size_t appendToken(char* buffer, size_t buffer_size, const char* token, const char* separator) {
  if (buffer_size == 0) {
    return 0;
  }

  size_t current_length = strlen(buffer);
  size_t separator_length = current_length > 0 ? strlen(separator) : 0;
  size_t token_length = strlen(token);

  if (current_length + separator_length + token_length >= buffer_size) {
    return current_length;
  }

  if (separator_length > 0) {
    strcat(buffer, separator);
  }
  strcat(buffer, token);
  return strlen(buffer);
}

inline void buildReasonList(uint16_t blockers, bool human_readable, char* buffer, size_t buffer_size) {
  if (buffer_size == 0) {
    return;
  }

  buffer[0] = '\0';

  if (blockers == 0) {
    strncpy(buffer, human_readable ? "All profile conditions satisfied" : "none", buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return;
  }

  size_t count = 0;
  const uint16_t* reasons = orderedBlockReasons(count);
  for (size_t i = 0; i < count; ++i) {
    if ((blockers & reasons[i]) == 0) {
      continue;
    }

    appendToken(buffer,
                buffer_size,
                human_readable ? blockReasonText(reasons[i]) : blockReasonCode(reasons[i]),
                human_readable ? "; " : ",");
  }
}

inline const char* firstActiveReminder(uint16_t blockers) {
  if (blockers == 0) {
    return "ready";
  }

  size_t count = 0;
  const uint16_t* reasons = orderedBlockReasons(count);
  for (size_t i = 0; i < count; ++i) {
    if ((blockers & reasons[i]) != 0) {
      return blockReasonCode(reasons[i]);
    }
  }

  return "ready";
}

#endif  // OPERATION_PROFILE_H
