#ifndef SMARTFARM_RULE_ENGINE_H
#define SMARTFARM_RULE_ENGINE_H

#include "../../include/types.h"
#include "conditions.h"

struct RuleEvaluation {
    RelayState aerator;
    RelayState pump;
    RelayState circulation;
    RelayState feeder;
    RelayState valve;
    RelayState light;
    RelayState spare_1;
    RelayState spare_2;
    SystemState system_state;
};

class RuleEngine {
public:
    RuleEvaluation evaluate(const SensorSnapshot& snapshot, const SpeciesProfile& profile) const;
};

#endif
