#include <cassert>
#include <cstring>
#include "operation_profile.h"

static ProfileEligibilityInputs baseInputs() {
  ProfileEligibilityInputs inputs = {};
  inputs.required_sensors_valid = true;
  inputs.sensor_fault_active = false;
  inputs.relay_test_passed = true;
  inputs.outputs_locked = false;
  inputs.sensor_test_mode_enabled = false;
  inputs.active_alarm = false;
  inputs.critical_condition_active = false;
  inputs.livestock_present_confirmed = false;
  return inputs;
}

int main() {
  {
    ProfileEligibilityInputs inputs = baseInputs();
    inputs.required_sensors_valid = false;
    inputs.sensor_fault_active = true;
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_NO_LIVESTOCK_TEST, inputs);
    assert(!evaluation.can_no_load_test);
    assert((evaluation.no_load_blockers & BLOCK_REQUIRED_SENSOR_INVALID) != 0);
    assert((evaluation.no_load_blockers & BLOCK_SENSOR_FAULT_ACTIVE) != 0);
    assert(evaluation.actual_profile == PROFILE_SENSOR_TEST);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    inputs.relay_test_passed = false;
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_NO_LIVESTOCK_TEST, inputs);
    assert(!evaluation.can_no_load_test);
    assert((evaluation.no_load_blockers & BLOCK_RELAY_TEST_NOT_PASSED) != 0);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    inputs.outputs_locked = true;
    inputs.sensor_test_mode_enabled = true;
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_PRODUCTION, inputs);
    assert(!evaluation.can_production);
    assert((evaluation.production_blockers & BLOCK_OUTPUTS_LOCKED) != 0);
    assert((evaluation.production_blockers & BLOCK_SENSOR_TEST_MODE_ENABLED) != 0);
    assert(evaluation.actual_profile == PROFILE_SENSOR_TEST);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_PRODUCTION, inputs);
    assert(!evaluation.can_production);
    assert((evaluation.production_blockers & BLOCK_LIVESTOCK_NOT_CONFIRMED) != 0);
    assert(evaluation.actual_profile == PROFILE_NO_LIVESTOCK_TEST);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_NO_LIVESTOCK_TEST, inputs);
    assert(evaluation.can_no_load_test);
    assert(evaluation.actual_profile == PROFILE_NO_LIVESTOCK_TEST);
    assert(evaluation.no_load_blockers == 0);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    inputs.critical_condition_active = true;
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_NO_LIVESTOCK_TEST, inputs);
    assert(!evaluation.can_no_load_test);
    assert((evaluation.no_load_blockers & BLOCK_CRITICAL_CONDITION_ACTIVE) != 0);
    assert(evaluation.actual_profile == PROFILE_SENSOR_TEST);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    inputs.livestock_present_confirmed = true;
    ProfileEvaluation evaluation = evaluateProfileRequest(PROFILE_PRODUCTION, inputs);
    assert(evaluation.can_production);
    assert(evaluation.actual_profile == PROFILE_PRODUCTION);
    assert(evaluation.production_blockers == 0);
  }

  {
    char buffer[256];
    buildReasonList(BLOCK_REQUIRED_SENSOR_INVALID | BLOCK_LIVESTOCK_NOT_CONFIRMED, false, buffer, sizeof(buffer));
    assert(std::strcmp(buffer, "required_sensor_invalid,livestock_not_confirmed") == 0);
  }

  {
    assert(isOperationProfileValue("SENSOR_TEST"));
    assert(isOperationProfileValue("NO_LIVESTOCK_TEST"));
    assert(isOperationProfileValue("PRODUCTION"));
    assert(!isOperationProfileValue("BROKEN"));
    OperationProfileId parsed_profile = PROFILE_SENSOR_TEST;
    assert(tryParseOperationProfile("PRODUCTION", parsed_profile));
    assert(parsed_profile == PROFILE_PRODUCTION);
    assert(!tryParseOperationProfile("BROKEN", parsed_profile));
    assert(parsed_profile == PROFILE_PRODUCTION);
    assert(operationProfileFromString("PRODUCTION") == PROFILE_PRODUCTION);
    assert(operationProfileFromString("UNKNOWN") == PROFILE_SENSOR_TEST);
  }

  {
    assert(isRelayTestStatusValue("NOT_STARTED"));
    assert(isRelayTestStatusValue("IN_PROGRESS"));
    assert(isRelayTestStatusValue("PASSED"));
    assert(isRelayTestStatusValue("FAILED"));
    assert(!isRelayTestStatusValue("BROKEN"));
    RelayTestStatusId parsed_status = RELAY_TEST_NOT_STARTED;
    assert(tryParseRelayTestStatus("PASSED", parsed_status));
    assert(parsed_status == RELAY_TEST_PASSED);
    assert(!tryParseRelayTestStatus("BROKEN", parsed_status));
    assert(parsed_status == RELAY_TEST_PASSED);
    assert(relayTestStatusFromString("PASSED") == RELAY_TEST_PASSED);
    assert(relayTestStatusFromString("UNKNOWN") == RELAY_TEST_NOT_STARTED);
  }

  {
    ProfileEligibilityInputs inputs = baseInputs();
    inputs.livestock_present_confirmed = true;
    OperationProfileId requested_profile = PROFILE_SENSOR_TEST;
    ProfileCommandUpdate profile_update = applyOperationProfileCommand("INVALID", requested_profile, inputs);
    assert(!profile_update.accepted);
    assert(profile_update.requested_profile == requested_profile);
    assert(profile_update.evaluation.actual_profile == PROFILE_SENSOR_TEST);

    profile_update = applyOperationProfileCommand("PRODUCTION", requested_profile, inputs);
    assert(profile_update.accepted);
    assert(profile_update.requested_profile == PROFILE_PRODUCTION);
    assert(profile_update.evaluation.actual_profile == PROFILE_PRODUCTION);

    RelayTestCommandUpdate relay_update = applyRelayTestCommand("INVALID", RELAY_TEST_NOT_STARTED);
    assert(!relay_update.accepted);
    assert(relay_update.requested_status == RELAY_TEST_NOT_STARTED);

    relay_update = applyRelayTestCommand("PASSED", RELAY_TEST_NOT_STARTED);
    assert(relay_update.accepted);
    assert(relay_update.requested_status == RELAY_TEST_PASSED);
  }

  return 0;
}
