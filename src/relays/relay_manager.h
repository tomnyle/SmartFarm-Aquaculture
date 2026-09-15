#ifndef SMARTFARM_RELAY_MANAGER_H
#define SMARTFARM_RELAY_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "relay_types.h"

class RelayManager {
public:
    RelayManager();

    void begin();
    bool setRelayState(const char* name, RelayState state);
    RelayState getRelayState(const char* name) const;
    const RelayChannelState* getChannels() const;
    uint8_t count() const;
    String toJson() const;

private:
    RelayChannelState states_[RELAY_COUNT];
    int indexOf(const char* name) const;
    void writePin(uint8_t index, RelayState state);
};

#endif
