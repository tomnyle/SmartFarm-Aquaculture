#include "rule_engine.h"

RuleEvaluation RuleEngine::evaluate(const SensorSnapshot& snapshot, const SpeciesProfile& profile) const {
    RuleEvaluation result = {
        RELAY_OFF, RELAY_OFF, RELAY_OFF, RELAY_OFF,
        RELAY_OFF, RELAY_OFF, RELAY_OFF, RELAY_OFF,
        SYSTEM_RUNNING
    };

    if (isBelow(snapshot.water_level, 0.5F) && profile.low_level_triggers_safe) {
        result.aerator = RELAY_ON;
        result.system_state = SYSTEM_SAFE;
        return result;
    }

    if (isBelow(snapshot.dissolved_oxygen, profile.dissolved_oxygen_min)) {
        result.aerator = RELAY_ON;
        result.pump = RELAY_ON;
    }

    if (isAbove(snapshot.temperature, profile.temperature_max)) {
        result.pump = RELAY_ON;
        result.circulation = RELAY_ON;
    }

    if (isBelow(snapshot.temperature, profile.temperature_min)) {
        result.circulation = RELAY_OFF;
    }

    if ((isSensorEnabled(snapshot.temperature) && !isSensorOk(snapshot.temperature)) ||
        (isSensorEnabled(snapshot.ph) && !isSensorOk(snapshot.ph)) ||
        (isSensorEnabled(snapshot.dissolved_oxygen) && !isSensorOk(snapshot.dissolved_oxygen))) {
        result.system_state = SYSTEM_ERROR;
    }

    return result;
}
