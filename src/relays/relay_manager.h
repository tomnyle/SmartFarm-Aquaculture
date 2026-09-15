#ifndef SRC_RELAYS_RELAY_MANAGER_H
#define SRC_RELAYS_RELAY_MANAGER_H

#include <Arduino.h>
#include "relay_names.h"
#include "relay_types.h"
#include "../utils/data_types.h"

class RelayManager {
 public:
  void begin();
  bool setRelay(const char* relayName, bool on);
  bool getRelay(const char* relayName) const;
  const RelaySnapshot* snapshots() const { return relays_; }
  void applySafeDefaults(bool aeratorOn);

 private:
  RelaySnapshot relays_[8] = {
      {relay_names::AERATOR, false}, {relay_names::PUMP, false}, {relay_names::CIRCULATION, false},
      {relay_names::FEEDER, false},  {relay_names::VALVE, false}, {relay_names::LIGHT, false},
      {relay_names::SPARE1, false},  {relay_names::SPARE2, false}};
  bool setIndex(uint8_t index, bool on);
};

#endif
