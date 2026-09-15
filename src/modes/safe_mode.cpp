#include "safe_mode.h"

void SafeMode::apply(RelayManager& relayManager) const {
  relayManager.applySafeDefaults(true);
}
