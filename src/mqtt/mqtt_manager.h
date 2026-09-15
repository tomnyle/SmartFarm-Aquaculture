#ifndef SMARTFARM_MQTT_MANAGER_H
#define SMARTFARM_MQTT_MANAGER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "../../include/types.h"
#include "../relays/relay_manager.h"

class MqttManager {
public:
    using RelayCommandHandler = void (*)(const char* relay_name, RelayState state);
    using ModeCommandHandler = void (*)(const char* mode);

    MqttManager();

    void begin(const char* device_id);
    void loop();
    bool ensureConnected();
    void publishDiscovery(const RelayManager& relay_manager, const SensorSnapshot& snapshot);
    void publishSensors(const SensorSnapshot& snapshot);
    void publishRelayStates(const RelayManager& relay_manager);
    void publishSystemState(SystemState state, const char* profile_id, const char* mode);
    void publishError(const char* message);
    bool isConnected() const;
    void setRelayCommandHandler(RelayCommandHandler handler);
    void setModeCommandHandler(ModeCommandHandler handler);

private:
    // Internal clients / Trạng thái nội bộ của MQTT client.
    WiFiClient wifi_client_;
    PubSubClient client_;
    String device_id_;
    RelayCommandHandler relay_handler_;
    ModeCommandHandler mode_handler_;
    bool discovery_published_;
    uint32_t last_connect_attempt_;

    static MqttManager* instance_;
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    void onMessage(char* topic, byte* payload, unsigned int length);
    void publishSensorDiscovery(const char* unique_id, const char* name, const String& state_topic, const char* unit, const char* icon);
    void publishRelayDiscovery(const char* relay_name, const String& state_topic, const String& command_topic);
};

#endif
