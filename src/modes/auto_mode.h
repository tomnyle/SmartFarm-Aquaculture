#ifndef SRC_MODES_AUTO_MODE_H
#define SRC_MODES_AUTO_MODE_H

#include "../rules/conditions.h"
#include "../relays/relay_manager.h"

class AutoMode {
 public:
  void apply(const RuleEvaluation& evaluation, RelayManager& relayManager) const;
};

#endif
