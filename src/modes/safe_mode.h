#ifndef SRC_MODES_SAFE_MODE_H
#define SRC_MODES_SAFE_MODE_H

#include "../relays/relay_manager.h"

class SafeMode {
 public:
  void apply(RelayManager& relayManager) const;
};

#endif
