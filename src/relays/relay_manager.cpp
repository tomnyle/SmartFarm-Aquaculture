#include <string.h>
#include "relay_manager.h"
#include "../config/relay_config.h"

void RelayManager::begin() {
  for (uint8_t i = 0; i < 8; ++i) {
    pinMode(relay_config::RELAY_PINS[i], OUTPUT);
    setIndex(i, false);
  }
}

bool RelayManager::setIndex(uint8_t index, bool on) {
  if (index >= 8) return false;
  relays_[index].state = on;
  digitalWrite(relay_config::RELAY_PINS[index], (relay_config::ACTIVE_HIGH == on) ? HIGH : LOW);
  return true;
}

bool RelayManager::setRelay(const char* relayName, bool on) {
  for (uint8_t i = 0; i < 8; ++i) {
    if (strcmp(relays_[i].name, relayName) == 0) {
      return setIndex(i, on);
    }
  }
  return false;
}

bool RelayManager::getRelay(const char* relayName) const {
  for (const auto& relay : relays_) {
    if (strcmp(relay.name, relayName) == 0) {
      return relay.state;
    }
  }
  return false;
}

void RelayManager::applySafeDefaults(bool aeratorOn) {
  setRelay(relay_names::AERATOR, aeratorOn);
  setRelay(relay_names::PUMP, false);
  setRelay(relay_names::CIRCULATION, false);
  setRelay(relay_names::FEEDER, false);
  setRelay(relay_names::VALVE, false);
  setRelay(relay_names::LIGHT, false);
  setRelay(relay_names::SPARE1, false);
  setRelay(relay_names::SPARE2, false);
}
