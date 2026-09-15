#ifndef SRC_MODES_MANUAL_MODE_H
#define SRC_MODES_MANUAL_MODE_H

#include "../relays/relay_manager.h"

class ManualMode {
 public:
  bool applyCommand(RelayManager& relayManager, const char* relayName, bool on) const;
};

#endif
