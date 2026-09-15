#include "rule_engine.h"
#include "../config/thresholds_config.h"

ConditionState RuleEngine::evaluateTemperature(const SpeciesProfile& profile, const SensorReading& reading) const {
  if (!reading.available) return ConditionState::CRITICAL;
  if (reading.value <= profile.temperature.criticalMin || reading.value >= profile.temperature.criticalMax) return ConditionState::CRITICAL;
  if (reading.value < profile.temperature.min || reading.value > profile.temperature.max) return ConditionState::WARNING;
  return ConditionState::NORMAL;
}

ConditionState RuleEngine::evaluatePH(const SpeciesProfile& profile, const SensorReading& reading) const {
  if (!reading.available) return ConditionState::CRITICAL;
  if (reading.value <= profile.ph.criticalMin || reading.value >= profile.ph.criticalMax) return ConditionState::CRITICAL;
  if (reading.value < profile.ph.min || reading.value > profile.ph.max) return ConditionState::WARNING;
  return ConditionState::NORMAL;
}

ConditionState RuleEngine::evaluateDO(const SpeciesProfile& profile, const SensorReading& reading) const {
  if (!reading.available) return ConditionState::CRITICAL;
  if (reading.value <= profile.dissolvedOxygen.criticalMin) return ConditionState::CRITICAL;
  if (reading.value < profile.dissolvedOxygen.min) return ConditionState::WARNING;
  return ConditionState::NORMAL;
}

ConditionState RuleEngine::evaluateWaterLevel(const SpeciesProfile& profile, const SensorReading& reading) const {
  if (!reading.available) return ConditionState::CRITICAL;
  if (reading.value <= profile.waterLevel.criticalMin) return ConditionState::CRITICAL;
  if (reading.value < profile.waterLevel.min || reading.value > profile.waterLevel.max) return ConditionState::WARNING;
  return ConditionState::NORMAL;
}

RuleEvaluation RuleEngine::evaluate(const SpeciesProfile& profile,
                                    const SensorReading& temperature,
                                    const SensorReading& ph,
                                    const SensorReading& dissolvedOxygen,
                                    const SensorReading& waterLevel) const {
  RuleEvaluation result{};
  result.conditions.temperature = evaluateTemperature(profile, temperature);
  result.conditions.ph = evaluatePH(profile, ph);
  result.conditions.dissolvedOxygen = evaluateDO(profile, dissolvedOxygen);
  result.conditions.waterLevel = evaluateWaterLevel(profile, waterLevel);

  result.aeratorOn = result.conditions.dissolvedOxygen != ConditionState::NORMAL;
  result.pumpOn = result.conditions.waterLevel != ConditionState::NORMAL;
  result.circulationOn = result.conditions.temperature != ConditionState::NORMAL;
  result.safeModeRequired = result.conditions.temperature == ConditionState::CRITICAL ||
                            result.conditions.ph == ConditionState::CRITICAL ||
                            result.conditions.dissolvedOxygen == ConditionState::CRITICAL ||
                            result.conditions.waterLevel == ConditionState::CRITICAL;
  result.reason = result.safeModeRequired ? "critical_condition" : "profile_rules";
  return result;
}
