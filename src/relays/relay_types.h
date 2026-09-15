#ifndef SRC_RELAYS_RELAY_TYPES_H
#define SRC_RELAYS_RELAY_TYPES_H

enum class RelayState {
  OFF = 0,
  ON = 1
};

enum class RelayType {
  AERATOR,
  PUMP,
  CIRCULATION,
  FEEDER,
  VALVE,
  LIGHT,
  SPARE1,
  SPARE2
};

#endif
