#ifndef SRC_RULES_RULE_ENGINE_H
#define SRC_RULES_RULE_ENGINE_H

#include "conditions.h"

class RuleEngine {
 public:
  RuleEvaluation evaluate(const SpeciesProfile& profile,
                          const SensorReading& temperature,
                          const SensorReading& ph,
                          const SensorReading& dissolvedOxygen,
                          const SensorReading& waterLevel) const;

 private:
  ConditionState evaluateTemperature(const SpeciesProfile& profile, const SensorReading& reading) const;
  ConditionState evaluatePH(const SpeciesProfile& profile, const SensorReading& reading) const;
  ConditionState evaluateDO(const SpeciesProfile& profile, const SensorReading& reading) const;
  ConditionState evaluateWaterLevel(const SpeciesProfile& profile, const SensorReading& reading) const;
};

#endif
