#ifndef SRC_MQTT_TOPICS_H
#define SRC_MQTT_TOPICS_H

#include <Arduino.h>
#include "../config/app_config.h"

namespace mqtt_topics {
inline String prefix() { return String(app_config::MQTT_BASE_TOPIC) + "/" + app_config::DEVICE_ID; }
inline String state() { return prefix() + "/state"; }
inline String sensor(const char* name) { return prefix() + "/sensor/" + name; }
inline String relay(const char* name) { return prefix() + "/relay/" + name; }
inline String relaySet(const char* name) { return prefix() + "/control/relay/" + name + "/set"; }
inline String mode() { return prefix() + "/system/mode"; }
inline String modeSet() { return prefix() + "/control/mode/set"; }
inline String profile() { return prefix() + "/system/profile"; }
inline String profileSet() { return prefix() + "/control/profile/set"; }
inline String systemState() { return prefix() + "/system/state"; }
inline String condition(const char* name) { return prefix() + "/conditions/" + name; }
inline String error() { return prefix() + "/status/error"; }
inline String scheduleSet() { return prefix() + "/control/schedule/set"; }
inline String availability() { return prefix() + "/status/availability"; }
}

#endif
