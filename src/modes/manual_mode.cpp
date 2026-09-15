#include "manual_mode.h"

bool ManualMode::applyCommand(RelayManager& relayManager, const char* relayName, bool on) const {
  return relayManager.setRelay(relayName, on);
}
