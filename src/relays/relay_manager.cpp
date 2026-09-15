#include <cstring>
#include "relay_manager.h"

RelayManager::RelayManager() {
    for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
        states_[i].name = RELAY_CONFIGS[i].name;
        states_[i].state = RELAY_CONFIGS[i].default_state;
        states_[i].enabled = RELAY_CONFIGS[i].enabled;
    }
}

void RelayManager::begin() {
    for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
        if (!RELAY_CONFIGS[i].enabled) {
            continue;
        }
        pinMode(RELAY_CONFIGS[i].pin, OUTPUT);
        writePin(i, states_[i].state);
    }
}

bool RelayManager::setRelayState(const char* name, RelayState state) {
    const int index = indexOf(name);
    if (index < 0 || !states_[index].enabled) {
        return false;
    }

    states_[index].state = state;
    writePin(static_cast<uint8_t>(index), state);
    return true;
}

RelayState RelayManager::getRelayState(const char* name) const {
    const int index = indexOf(name);
    if (index < 0) {
        return RELAY_ERROR;
    }
    return states_[index].state;
}

const RelayChannelState* RelayManager::getChannels() const {
    return states_;
}

uint8_t RelayManager::count() const {
    return RELAY_COUNT;
}

String RelayManager::toJson() const {
    StaticJsonDocument<512> document;
    for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
        document[states_[i].name] = states_[i].state == RELAY_ON ? "ON" : "OFF";
    }

    String payload;
    serializeJson(document, payload);
    return payload;
}

int RelayManager::indexOf(const char* name) const {
    for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
        if (strcmp(states_[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void RelayManager::writePin(uint8_t index, RelayState state) {
    const bool logical_on = state == RELAY_ON;
    const bool pin_state = RELAY_CONFIGS[index].active_high ? logical_on : !logical_on;
    digitalWrite(RELAY_CONFIGS[index].pin, pin_state ? HIGH : LOW);
}
