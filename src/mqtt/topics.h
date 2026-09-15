#ifndef SMARTFARM_TOPICS_H
#define SMARTFARM_TOPICS_H

#include <Arduino.h>

namespace Topics {
    inline String base(const char* device_id) { return String("smartfarm/aquaculture/") + device_id + "/"; }
    inline String state(const char* device_id) { return base(device_id) + "state"; }
    inline String sensorAll(const char* device_id) { return base(device_id) + "sensor/all"; }
    inline String sensorTemperature(const char* device_id) { return base(device_id) + "sensor/temperature"; }
    inline String sensorPh(const char* device_id) { return base(device_id) + "sensor/ph"; }
    inline String sensorDo(const char* device_id) { return base(device_id) + "sensor/do"; }
    inline String sensorWaterLevel(const char* device_id) { return base(device_id) + "sensor/water_level"; }
    inline String sensorEc(const char* device_id) { return base(device_id) + "sensor/ec"; }
    inline String sensorOrp(const char* device_id) { return base(device_id) + "sensor/orp"; }
    inline String relayAll(const char* device_id) { return base(device_id) + "relay/all"; }
    inline String relay(const char* device_id, const char* name) { return base(device_id) + "relay/" + name; }
    inline String controlRelayPrefix(const char* device_id) { return base(device_id) + "control/relay/"; }
    inline String controlMode(const char* device_id) { return base(device_id) + "control/mode"; }
    inline String error(const char* device_id) { return base(device_id) + "status/error"; }
}

#endif
