#ifndef SRC_MQTT_MQTT_MANAGER_H
#define SRC_MQTT_MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFi.h>
#include "ha_discovery.h"
#include "../modes/operation_mode.h"
#include "../relays/relay_manager.h"
#include "../sensors/sensor_manager.h"
#include "../system/system_state.h"
#include "../utils/data_types.h"

class MQTTManager {
 public:
  MQTTManager();
  void begin();
  void loop();
  bool connected() const;
  void publishState(const SystemState& systemState, const SensorManager& sensors, const RelayManager& relays) const;
  void publishDiscovery(const SystemState& state) const;
  bool consumeRelayCommand(String& relayName, bool& relayState);
  bool consumeModeCommand(OperationMode& mode);
  bool consumeProfileCommand(String& profile);
  bool consumeScheduleCommand(String& payload);

 private:
  static MQTTManager* instance_;
  static void callback(char* topic, byte* payload, unsigned int length);
  void handleMessage(const char* topic, const String& payload);
  void subscribeTopics();

  mutable WiFiClient wifiClient_;
  mutable PubSubClient client_;
  HADiscovery discovery_;
  String pendingRelayName_;
  bool pendingRelayState_ = false;
  bool hasRelayCommand_ = false;
  OperationMode pendingMode_ = OperationMode::AUTO;
  bool hasModeCommand_ = false;
  String pendingProfile_;
  bool hasProfileCommand_ = false;
  String pendingSchedule_;
  bool hasScheduleCommand_ = false;
};

#endif
