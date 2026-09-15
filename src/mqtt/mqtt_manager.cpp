#include <cstring>
#include "mqtt_manager.h"
#include "topics.h"
#include "../config/app_config.h"

MqttManager* MqttManager::instance_ = nullptr;

MqttManager::MqttManager()
    : client_(wifi_client_),
      relay_handler_(nullptr),
      mode_handler_(nullptr),
      discovery_published_(false),
      last_connect_attempt_(0) {
    instance_ = this;
}

void MqttManager::begin(const char* device_id) {
    device_id_ = device_id;
    client_.setServer(MQTT_BROKER, MQTT_PORT);
    client_.setBufferSize(1024);
    client_.setCallback(staticCallback);
}

void MqttManager::loop() {
    client_.loop();
}

bool MqttManager::ensureConnected() {
    if (client_.connected()) {
        return true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if ((millis() - last_connect_attempt_) < MQTT_RECONNECT_INTERVAL_MS) {
        return false;
    }
    last_connect_attempt_ = millis();

    if (!client_.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
        return false;
    }

    discovery_published_ = false;
    client_.subscribe(Topics::controlMode(device_id_.c_str()).c_str());
    client_.subscribe((Topics::controlRelayPrefix(device_id_.c_str()) + "#").c_str());
    return true;
}

void MqttManager::publishDiscovery(const RelayManager& relay_manager, const SensorSnapshot& snapshot) {
    if (!client_.connected() || discovery_published_) {
        return;
    }

    if (snapshot.temperature.status != SENSOR_STATUS_DISABLED) {
        publishSensorDiscovery("temperature", "Water Temperature", Topics::sensorTemperature(device_id_.c_str()), "°C", "mdi:thermometer", "temperature");
    }
    if (snapshot.ph.status != SENSOR_STATUS_DISABLED) {
        publishSensorDiscovery("ph", "Water pH", Topics::sensorPh(device_id_.c_str()), "pH", "mdi:test-tube");
    }
    if (snapshot.dissolved_oxygen.status != SENSOR_STATUS_DISABLED) {
        publishSensorDiscovery("do", "Dissolved Oxygen", Topics::sensorDo(device_id_.c_str()), "mg/L", "mdi:water");
    }
    if (snapshot.water_level.status != SENSOR_STATUS_DISABLED) {
        publishBinarySensorDiscovery("water_level", "Water Level Low", Topics::sensorWaterLevel(device_id_.c_str()), "LOW", "HIGH", "mdi:waves-arrow-up");
    }
    if (snapshot.ec.status != SENSOR_STATUS_DISABLED) {
        publishSensorDiscovery("ec", "EC/TDS", Topics::sensorEc(device_id_.c_str()), "mS/cm", "mdi:flash");
    }
    if (snapshot.orp.status != SENSOR_STATUS_DISABLED) {
        publishSensorDiscovery("orp", "ORP", Topics::sensorOrp(device_id_.c_str()), "mV", "mdi:chart-line");
    }

    for (uint8_t i = 0; i < relay_manager.count(); ++i) {
        const RelayChannelState& channel = relay_manager.getChannels()[i];
        if (!channel.enabled) {
            continue;
        }
        publishRelayDiscovery(
            channel.name,
            Topics::relay(device_id_.c_str(), channel.name),
            Topics::controlRelayPrefix(device_id_.c_str()) + channel.name
        );
    }

    discovery_published_ = true;
}

void MqttManager::publishSensors(const SensorSnapshot& snapshot) {
    if (!client_.connected()) {
        return;
    }

    StaticJsonDocument<512> document;
    if (snapshot.temperature.status != SENSOR_STATUS_DISABLED) {
        document["temperature"] = snapshot.temperature.value;
    }
    if (snapshot.ph.status != SENSOR_STATUS_DISABLED) {
        document["ph"] = snapshot.ph.value;
    }
    if (snapshot.dissolved_oxygen.status != SENSOR_STATUS_DISABLED) {
        document["do"] = snapshot.dissolved_oxygen.value;
    }
    if (snapshot.water_level.status != SENSOR_STATUS_DISABLED) {
        document["water_level"] = snapshot.water_level.value > 0.5F ? "HIGH" : "LOW";
    }
    if (snapshot.ec.status != SENSOR_STATUS_DISABLED) {
        document["ec"] = snapshot.ec.value;
    }
    if (snapshot.orp.status != SENSOR_STATUS_DISABLED) {
        document["orp"] = snapshot.orp.value;
    }

    String payload;
    serializeJson(document, payload);
    client_.publish(Topics::sensorAll(device_id_.c_str()).c_str(), payload.c_str(), true);

    if (snapshot.temperature.status != SENSOR_STATUS_DISABLED) {
        client_.publish(Topics::sensorTemperature(device_id_.c_str()).c_str(), String(snapshot.temperature.value, 2).c_str(), true);
    }
    if (snapshot.ph.status != SENSOR_STATUS_DISABLED) {
        client_.publish(Topics::sensorPh(device_id_.c_str()).c_str(), String(snapshot.ph.value, 2).c_str(), true);
    }
    if (snapshot.dissolved_oxygen.status != SENSOR_STATUS_DISABLED) {
        client_.publish(Topics::sensorDo(device_id_.c_str()).c_str(), String(snapshot.dissolved_oxygen.value, 2).c_str(), true);
    }
    if (snapshot.water_level.status != SENSOR_STATUS_DISABLED) {
        client_.publish(Topics::sensorWaterLevel(device_id_.c_str()).c_str(), snapshot.water_level.value > 0.5F ? "HIGH" : "LOW", true);
    }
    if (snapshot.ec.status != SENSOR_STATUS_DISABLED) {
        client_.publish(Topics::sensorEc(device_id_.c_str()).c_str(), String(snapshot.ec.value, 2).c_str(), true);
    }
    if (snapshot.orp.status != SENSOR_STATUS_DISABLED) {
        client_.publish(Topics::sensorOrp(device_id_.c_str()).c_str(), String(snapshot.orp.value, 2).c_str(), true);
    }
}

void MqttManager::publishRelayStates(const RelayManager& relay_manager) {
    if (!client_.connected()) {
        return;
    }

    const String payload = relay_manager.toJson();
    client_.publish(Topics::relayAll(device_id_.c_str()).c_str(), payload.c_str(), true);

    for (uint8_t i = 0; i < relay_manager.count(); ++i) {
        const RelayChannelState& channel = relay_manager.getChannels()[i];
        if (!channel.enabled) {
            continue;
        }
        client_.publish(
            Topics::relay(device_id_.c_str(), channel.name).c_str(),
            channel.state == RELAY_ON ? "ON" : "OFF",
            true
        );
    }
}

void MqttManager::publishSystemState(SystemState state, const char* profile_id, const char* mode) {
    if (!client_.connected()) {
        return;
    }

    StaticJsonDocument<256> document;
    document["device_id"] = device_id_;
    document["profile"] = profile_id;
    document["mode"] = mode;
    document["state"] = state == SYSTEM_SAFE ? "SAFE" : state == SYSTEM_ERROR ? "ERROR" : state == SYSTEM_RUNNING ? "RUNNING" : state == SYSTEM_READY ? "READY" : "INIT";
    document["uptime_ms"] = millis();

    String payload;
    serializeJson(document, payload);
    client_.publish(Topics::state(device_id_.c_str()).c_str(), payload.c_str(), true);
}

void MqttManager::publishError(const char* message) {
    if (!client_.connected()) {
        return;
    }
    client_.publish(Topics::error(device_id_.c_str()).c_str(), message, true);
}

bool MqttManager::isConnected() const {
    return client_.connected();
}

void MqttManager::setRelayCommandHandler(RelayCommandHandler handler) {
    relay_handler_ = handler;
}

void MqttManager::setModeCommandHandler(ModeCommandHandler handler) {
    mode_handler_ = handler;
}

void MqttManager::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (instance_ != nullptr) {
        instance_->onMessage(topic, payload, length);
    }
}

void MqttManager::onMessage(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int index = 0; index < length; ++index) {
        message += static_cast<char>(payload[index]);
    }

    const String topic_string(topic);
    const String relay_prefix = Topics::controlRelayPrefix(device_id_.c_str());

    if (topic_string == Topics::controlMode(device_id_.c_str()) && mode_handler_ != nullptr) {
        mode_handler_(message.c_str());
        return;
    }

    if (topic_string.startsWith(relay_prefix) && relay_handler_ != nullptr) {
        const String relay_name = topic_string.substring(relay_prefix.length());
        relay_handler_(relay_name.c_str(), message == "ON" ? RELAY_ON : RELAY_OFF);
    }
}

void MqttManager::publishSensorDiscovery(const char* unique_id, const char* name, const String& state_topic, const char* unit, const char* icon, const char* device_class) {
    StaticJsonDocument<384> document;
    document["name"] = name;
    document["unique_id"] = String(device_id_) + "_" + unique_id;
    document["state_topic"] = state_topic;
    if (unit != nullptr && strlen(unit) > 0) {
        document["unit_of_measurement"] = unit;
    }
    if (device_class != nullptr && strlen(device_class) > 0) {
        document["device_class"] = device_class;
    }
    document["icon"] = icon;
    document["device"]["identifiers"][0] = device_id_;
    document["device"]["name"] = APP_DEVICE_NAME;

    String payload;
    serializeJson(document, payload);
    const String config_topic = String("homeassistant/sensor/") + device_id_ + "/" + unique_id + "/config";
    client_.publish(config_topic.c_str(), payload.c_str(), true);
}

void MqttManager::publishBinarySensorDiscovery(const char* unique_id, const char* name, const String& state_topic, const char* payload_on, const char* payload_off, const char* icon) {
    StaticJsonDocument<384> document;
    document["name"] = name;
    document["unique_id"] = String(device_id_) + "_" + unique_id;
    document["state_topic"] = state_topic;
    document["payload_on"] = payload_on;
    document["payload_off"] = payload_off;
    document["icon"] = icon;
    document["device"]["identifiers"][0] = device_id_;
    document["device"]["name"] = APP_DEVICE_NAME;

    String payload;
    serializeJson(document, payload);
    const String config_topic = String("homeassistant/binary_sensor/") + device_id_ + "/" + unique_id + "/config";
    client_.publish(config_topic.c_str(), payload.c_str(), true);
}

void MqttManager::publishRelayDiscovery(const char* relay_name, const String& state_topic, const String& command_topic) {
    StaticJsonDocument<384> document;
    document["name"] = relay_name;
    document["unique_id"] = String(device_id_) + "_relay_" + relay_name;
    document["state_topic"] = state_topic;
    document["command_topic"] = command_topic;
    document["payload_on"] = "ON";
    document["payload_off"] = "OFF";
    document["device"]["identifiers"][0] = device_id_;
    document["device"]["name"] = APP_DEVICE_NAME;

    String payload;
    serializeJson(document, payload);
    const String config_topic = String("homeassistant/switch/") + device_id_ + "/" + relay_name + "/config";
    client_.publish(config_topic.c_str(), payload.c_str(), true);
}
