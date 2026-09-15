#ifndef SRC_MQTT_HA_DISCOVERY_H
#define SRC_MQTT_HA_DISCOVERY_H

#include <PubSubClient.h>
#include "../system/system_state.h"

class HADiscovery {
 public:
  void publishAll(PubSubClient& client, const SystemState& state) const;

 private:
  void publishSensor(PubSubClient& client, const char* objectId, const char* name,
                     const String& stateTopic, const char* unit, const char* icon) const;
  void publishSwitch(PubSubClient& client, const char* objectId, const char* name,
                     const char* relayName, const char* icon) const;
  void publishSelect(PubSubClient& client, const char* objectId, const char* name,
                     const String& stateTopic, const String& commandTopic,
                     const char* const* options, uint8_t optionCount, const char* icon) const;
  void publishText(PubSubClient& client, const char* objectId, const char* name,
                   const String& stateTopic, const char* icon) const;
};

#endif
