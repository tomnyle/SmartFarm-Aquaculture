#include "auto_mode.h"
#include "../relays/relay_names.h"

void AutoMode::apply(const RuleEvaluation& evaluation, RelayManager& relayManager) const {
  relayManager.setRelay(relay_names::AERATOR, evaluation.aeratorOn);
  relayManager.setRelay(relay_names::PUMP, evaluation.pumpOn);
  relayManager.setRelay(relay_names::CIRCULATION, evaluation.circulationOn);
}
