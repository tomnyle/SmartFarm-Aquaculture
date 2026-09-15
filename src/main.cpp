#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "app_config.h"
#include "pins.h"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

struct SensorData {
  float water_temp;
  float ph;
  float do_value;
  float water_level;
  float turbidity;
  float air_temp;
  float humidity;
  float light;
  float co2;
  uint32_t last_read;
};

struct OutputState {
  bool pump;
  bool aerator;
  bool circulation;
  bool feeder;
  bool spare1;
  bool spare2;
};

struct RelayDefinition {
  const char* name;
  const char* state_topic;
  const char* command_topic;
  const char* discovery_topic;
  const char* discovery_name;
  int pin;
  bool* state_ref;
};

SensorData sensors = {};
OutputState outputs = {};

uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;

char current_mode[16] = DEFAULT_MODE;
char current_profile[16] = "Shrimp";

void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void apply_rules();
void publish_sensor_data();
void publish_output_state();
void set_output(const char* name, bool state);
const char* status_range(float value, float min_value, float max_value);
const char* status_min_with_critical(float value, float min_value, float critical_value);
const char* status_max(float value, float max_value);
void publish_discovery_sensor(
  const char* topic,
  const char* unique_id,
  const char* name,
  const char* state_topic,
  const char* unit = nullptr,
  float min_value = NAN,
  float max_value = NAN,
  float critical_value = NAN
);
void publish_discovery_text_sensor(
  const char* topic,
  const char* unique_id,
  const char* name,
  const char* state_topic
);

RelayDefinition relay_definitions[] = {
  {"pump", MQTT_TOPIC_PUMP_STATE, MQTT_TOPIC_PUMP_COMMAND, "homeassistant/switch/aquaculture_pump/config", "Aquaculture Pump", PUMP_PIN, &outputs.pump},
  {"aerator", MQTT_TOPIC_AERATOR_STATE, MQTT_TOPIC_AERATOR_COMMAND, "homeassistant/switch/aquaculture_aerator/config", "Aquaculture Aerator", AERATOR_PIN, &outputs.aerator},
  {"circulation", MQTT_TOPIC_CIRCULATION_STATE, MQTT_TOPIC_CIRCULATION_COMMAND, "homeassistant/switch/aquaculture_circulation/config", "Aquaculture Circulation", CIRCULATION_PIN, &outputs.circulation},
  {"feeder", MQTT_TOPIC_FEEDER_STATE, MQTT_TOPIC_FEEDER_COMMAND, "homeassistant/switch/aquaculture_feeder/config", "Aquaculture Feeder", FEEDER_PIN, &outputs.feeder},
  {"spare1", MQTT_TOPIC_SPARE1_STATE, MQTT_TOPIC_SPARE1_COMMAND, "homeassistant/switch/aquaculture_spare1/config", "Aquaculture Spare 1", SPARE1_PIN, &outputs.spare1},
  {"spare2", MQTT_TOPIC_SPARE2_STATE, MQTT_TOPIC_SPARE2_COMMAND, "homeassistant/switch/aquaculture_spare2/config", "Aquaculture Spare 2", SPARE2_PIN, &outputs.spare2},
};

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");

  for (size_t i = 0; i < sizeof(relay_definitions) / sizeof(relay_definitions[0]); i++) {
    pinMode(relay_definitions[i].pin, OUTPUT);
    digitalWrite(relay_definitions[i].pin, LOW);
    *relay_definitions[i].state_ref = false;
  }

  setup_wifi();
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(1024);

  Serial.println("===== Initialization Complete =====\n");
}

void loop() {
  if (!mqtt_client.connected()) {
    reconnect_mqtt();
  } else {
    mqtt_client.loop();
  }

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  uint32_t now = millis();

  if (now - last_sensor_read >= SENSOR_READ_INTERVAL) {
    read_sensors();
    last_sensor_read = now;
  }

  if (now - last_rule_engine >= RULE_ENGINE_INTERVAL && strcmp(current_mode, "AUTO") == 0) {
    apply_rules();
    last_rule_engine = now;
  }

  if (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL && mqtt_client.connected()) {
    publish_sensor_data();
    publish_output_state();
    Serial.println("[MQTT] Data published successfully");
    last_mqtt_publish = now;
  }

  delay(10);
}

void setup_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("[OK] WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("[WARN] WiFi connection timeout, retrying...");
  }
}

void reconnect_mqtt() {
  if (mqtt_client.connected()) {
    return;
  }

  static uint32_t last_reconnect_attempt = 0;
  uint32_t now = millis();
  if (now - last_reconnect_attempt < MQTT_RECONNECT_INTERVAL) {
    return;
  }
  last_reconnect_attempt = now;

  Serial.print("[MQTT] Connecting to: ");
  Serial.println(MQTT_BROKER);

  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("[OK] MQTT Connected!");

    for (size_t i = 0; i < sizeof(relay_definitions) / sizeof(relay_definitions[0]); i++) {
      mqtt_client.subscribe(relay_definitions[i].command_topic);
    }
    mqtt_client.subscribe(MQTT_TOPIC_MODE_SET);
    mqtt_client.subscribe(MQTT_TOPIC_PROFILE_SET);

    publish_mqtt_discovery();
  } else {
    Serial.print("[WARN] MQTT connection failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

void publish_discovery_sensor(
  const char* topic,
  const char* unique_id,
  const char* name,
  const char* state_topic,
  const char* unit,
  float min_value,
  float max_value,
  float critical_value
) {
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = unique_id;
  doc["state_topic"] = state_topic;
  doc["value_template"] = "{{ value | float(0) }}";
  if (unit != nullptr) {
    doc["unit_of_measurement"] = unit;
  }
  if (!isnan(min_value)) {
    doc["min"] = min_value;
  }
  if (!isnan(max_value)) {
    doc["max"] = max_value;
  }
  if (!isnan(critical_value)) {
    doc["critical"] = critical_value;
  }
  doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
  doc["device"]["name"] = "Aquaculture Controller";

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(topic, payload.c_str(), true);
}

void publish_discovery_text_sensor(
  const char* topic,
  const char* unique_id,
  const char* name,
  const char* state_topic
) {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = unique_id;
  doc["state_topic"] = state_topic;
  doc["icon"] = "mdi:information-outline";
  doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
  doc["device"]["name"] = "Aquaculture Controller";

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(topic, payload.c_str(), true);
}

void publish_mqtt_discovery() {
  Serial.println("[HA Discovery] Publishing entity discoveries...");

  publish_discovery_sensor("homeassistant/sensor/aquaculture_water_temp/config", "aquaculture_water_temp", "Aquaculture Water Temp", MQTT_TOPIC_WATER_TEMP, "°C", TEMP_ALERT_LOW, TEMP_ALERT_HIGH);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_water_temp_status/config", "aquaculture_water_temp_status", "Aquaculture Water Temp Status", MQTT_TOPIC_WATER_TEMP_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_ph/config", "aquaculture_ph", "Aquaculture pH", MQTT_TOPIC_PH, nullptr, PH_ALERT_LOW, PH_ALERT_HIGH);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_ph_status/config", "aquaculture_ph_status", "Aquaculture pH Status", MQTT_TOPIC_PH_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_do/config", "aquaculture_do", "Aquaculture Dissolved Oxygen", MQTT_TOPIC_DO, "mg/L", DO_ALERT_LOW, NAN, DO_CRITICAL_LOW);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_do_status/config", "aquaculture_do_status", "Aquaculture Dissolved Oxygen Status", MQTT_TOPIC_DO_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_water_level/config", "aquaculture_water_level", "Aquaculture Water Level", MQTT_TOPIC_WATER_LEVEL, "%", WATER_LEVEL_ALERT_LOW, WATER_LEVEL_ALERT_HIGH);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_water_level_status/config", "aquaculture_water_level_status", "Aquaculture Water Level Status", MQTT_TOPIC_WATER_LEVEL_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_turbidity/config", "aquaculture_turbidity", "Aquaculture Turbidity", MQTT_TOPIC_TURBIDITY, "NTU", NAN, TURBIDITY_ALERT_HIGH);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_turbidity_status/config", "aquaculture_turbidity_status", "Aquaculture Turbidity Status", MQTT_TOPIC_TURBIDITY_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_air_temp/config", "aquaculture_air_temp", "Aquaculture Air Temperature", MQTT_TOPIC_AIR_TEMP, "°C");
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_air_temp_status/config", "aquaculture_air_temp_status", "Aquaculture Air Temperature Status", MQTT_TOPIC_AIR_TEMP_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_humidity/config", "aquaculture_humidity", "Aquaculture Humidity", MQTT_TOPIC_HUMIDITY, "%", HUMIDITY_ALERT_LOW, HUMIDITY_ALERT_HIGH);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_humidity_status/config", "aquaculture_humidity_status", "Aquaculture Humidity Status", MQTT_TOPIC_HUMIDITY_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_light/config", "aquaculture_light", "Aquaculture Light", MQTT_TOPIC_LIGHT, "lux", LIGHT_ALERT_LOW);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_light_status/config", "aquaculture_light_status", "Aquaculture Light Status", MQTT_TOPIC_LIGHT_STATUS);

  publish_discovery_sensor("homeassistant/sensor/aquaculture_co2/config", "aquaculture_co2", "Aquaculture CO2", MQTT_TOPIC_CO2, "ppm", NAN, CO2_ALERT_HIGH);
  publish_discovery_text_sensor("homeassistant/sensor/aquaculture_co2_status/config", "aquaculture_co2_status", "Aquaculture CO2 Status", MQTT_TOPIC_CO2_STATUS);

  for (size_t i = 0; i < sizeof(relay_definitions) / sizeof(relay_definitions[0]); i++) {
    StaticJsonDocument<512> doc;
    doc["name"] = relay_definitions[i].discovery_name;
    doc["unique_id"] = String("aquaculture_") + relay_definitions[i].name;
    doc["state_topic"] = relay_definitions[i].state_topic;
    doc["command_topic"] = relay_definitions[i].command_topic;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish(relay_definitions[i].discovery_topic, payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Mode";
    doc["unique_id"] = "aquaculture_mode";
    doc["state_topic"] = MQTT_TOPIC_MODE_STATE;
    doc["command_topic"] = MQTT_TOPIC_MODE_SET;
    doc["options"][0] = "AUTO";
    doc["options"][1] = "MANUAL";
    doc["options"][2] = "SCHEDULE";
    doc["options"][3] = "SAFE";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/select/aquaculture_mode/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Profile";
    doc["unique_id"] = "aquaculture_profile";
    doc["state_topic"] = MQTT_TOPIC_PROFILE_STATE;
    doc["command_topic"] = MQTT_TOPIC_PROFILE_SET;
    doc["options"][0] = "Koi";
    doc["options"][1] = "Catfish";
    doc["options"][2] = "Shrimp";
    doc["options"][3] = "Tilapia";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/select/aquaculture_profile/config", payload.c_str(), true);
  }

  Serial.println("[OK] All discoveries published!");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("[MQTT] Message received on: ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(message);

  for (size_t i = 0; i < sizeof(relay_definitions) / sizeof(relay_definitions[0]); i++) {
    if (strcmp(topic, relay_definitions[i].command_topic) == 0) {
      strcpy(current_mode, "MANUAL");
      set_output(relay_definitions[i].name, message == "ON");
      return;
    }
  }

  if (strcmp(topic, MQTT_TOPIC_MODE_SET) == 0) {
    if (message == "AUTO" || message == "MANUAL" || message == "SCHEDULE" || message == "SAFE") {
      strncpy(current_mode, message.c_str(), sizeof(current_mode) - 1);
      current_mode[sizeof(current_mode) - 1] = '\0';
      Serial.print("[CONFIG] Mode changed to: ");
      Serial.println(current_mode);
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_PROFILE_SET) == 0) {
    if (message == "Koi" || message == "Catfish" || message == "Shrimp" || message == "Tilapia") {
      strncpy(current_profile, message.c_str(), sizeof(current_profile) - 1);
      current_profile[sizeof(current_profile) - 1] = '\0';
      Serial.print("[CONFIG] Profile changed to: ");
      Serial.println(current_profile);
    }
  }
}

void read_sensors() {
  // Fake data until physical sensors are connected
  sensors.water_temp = 28.5f;
  sensors.ph = 7.8f;
  sensors.do_value = 6.2f;
  sensors.water_level = 85.0f;
  sensors.turbidity = 2.5f;
  sensors.air_temp = 25.0f;
  sensors.humidity = 65.0f;
  sensors.light = 500.0f;
  sensors.co2 = 0.5f;
  sensors.last_read = millis();

  Serial.println("===== SENSOR READINGS =====");
  Serial.printf("Water Temp: %.2f°C\n", sensors.water_temp);
  Serial.printf("pH: %.2f\n", sensors.ph);
  Serial.printf("DO: %.2f mg/L\n", sensors.do_value);
  Serial.printf("Water Level: %.2f%%\n", sensors.water_level);
  Serial.printf("Turbidity: %.2f NTU\n", sensors.turbidity);
  Serial.printf("Air Temp: %.2f°C\n", sensors.air_temp);
  Serial.printf("Humidity: %.2f%%\n", sensors.humidity);
  Serial.printf("Light: %.2f lux\n", sensors.light);
  Serial.printf("CO2: %.2f ppm\n", sensors.co2);
}

const char* status_range(float value, float min_value, float max_value) {
  if (value < min_value || value > max_value) {
    return "CRITICAL";
  }

  float margin = (max_value - min_value) * 0.1f;
  if (value <= min_value + margin || value >= max_value - margin) {
    return "WARNING";
  }
  return "NORMAL";
}

const char* status_min_with_critical(float value, float min_value, float critical_value) {
  if (value < critical_value) {
    return "CRITICAL";
  }
  if (value < min_value) {
    return "WARNING";
  }
  return "NORMAL";
}

const char* status_max(float value, float max_value) {
  if (value > max_value) {
    return "CRITICAL";
  }
  if (value >= max_value * 0.9f) {
    return "WARNING";
  }
  return "NORMAL";
}

void apply_rules() {
  bool pump_on = false;
  bool aerator_on = false;
  bool circulation_on = false;

  if (sensors.water_temp > AUTO_PUMP_TEMP_HIGH) {
    pump_on = true;
  }

  if (sensors.water_temp > AUTO_CIRCULATION_TEMP_HIGH) {
    circulation_on = true;
  }

  if (sensors.do_value < AUTO_PUMP_DO_LOW) {
    pump_on = true;
    aerator_on = true;
  }

  if (sensors.do_value < DO_CRITICAL_LOW) {
    pump_on = true;
    aerator_on = true;
    circulation_on = true;
  }

  if (sensors.co2 > AUTO_PUMP_CO2_HIGH) {
    pump_on = true;
  }

  set_output("pump", pump_on);
  set_output("aerator", aerator_on);
  set_output("circulation", circulation_on);
}

void set_output(const char* name, bool state) {
  for (size_t i = 0; i < sizeof(relay_definitions) / sizeof(relay_definitions[0]); i++) {
    if (strcmp(name, relay_definitions[i].name) == 0) {
      *relay_definitions[i].state_ref = state;
      digitalWrite(relay_definitions[i].pin, state ? HIGH : LOW);
      Serial.print("[OUTPUT] ");
      Serial.print(name);
      Serial.println(state ? " ON" : " OFF");
      return;
    }
  }
}

void publish_sensor_data() {
  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP, String(sensors.water_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP_MIN, String(TEMP_ALERT_LOW).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP_MAX, String(TEMP_ALERT_HIGH).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP_STATUS, status_range(sensors.water_temp, TEMP_ALERT_LOW, TEMP_ALERT_HIGH), true);

  mqtt_client.publish(MQTT_TOPIC_PH, String(sensors.ph, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH_MIN, String(PH_ALERT_LOW, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH_MAX, String(PH_ALERT_HIGH, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH_STATUS, status_range(sensors.ph, PH_ALERT_LOW, PH_ALERT_HIGH), true);

  mqtt_client.publish(MQTT_TOPIC_DO, String(sensors.do_value, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO_MIN, String(DO_ALERT_LOW, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO_CRITICAL, String(DO_CRITICAL_LOW, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO_STATUS, status_min_with_critical(sensors.do_value, DO_ALERT_LOW, DO_CRITICAL_LOW), true);

  mqtt_client.publish(MQTT_TOPIC_WATER_LEVEL, String(sensors.water_level, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_LEVEL_MIN, String(WATER_LEVEL_ALERT_LOW).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_LEVEL_MAX, String(WATER_LEVEL_ALERT_HIGH).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_LEVEL_STATUS, status_range(sensors.water_level, WATER_LEVEL_ALERT_LOW, WATER_LEVEL_ALERT_HIGH), true);

  mqtt_client.publish(MQTT_TOPIC_TURBIDITY, String(sensors.turbidity, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY_MAX, String(TURBIDITY_ALERT_HIGH, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY_STATUS, status_max(sensors.turbidity, TURBIDITY_ALERT_HIGH), true);

  mqtt_client.publish(MQTT_TOPIC_AIR_TEMP, String(sensors.air_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_AIR_TEMP_STATUS, "NORMAL", true);

  mqtt_client.publish(MQTT_TOPIC_HUMIDITY, String(sensors.humidity, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY_MIN, String(HUMIDITY_ALERT_LOW).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY_MAX, String(HUMIDITY_ALERT_HIGH).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY_STATUS, status_range(sensors.humidity, HUMIDITY_ALERT_LOW, HUMIDITY_ALERT_HIGH), true);

  mqtt_client.publish(MQTT_TOPIC_LIGHT, String(sensors.light, 0).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_LIGHT_MIN, String(LIGHT_ALERT_LOW).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_LIGHT_STATUS, sensors.light < LIGHT_ALERT_LOW ? "WARNING" : "NORMAL", true);

  mqtt_client.publish(MQTT_TOPIC_CO2, String(sensors.co2, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_CO2_MAX, String(CO2_ALERT_HIGH, 1).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_CO2_STATUS, status_max(sensors.co2, CO2_ALERT_HIGH), true);
}

void publish_output_state() {
  for (size_t i = 0; i < sizeof(relay_definitions) / sizeof(relay_definitions[0]); i++) {
    mqtt_client.publish(relay_definitions[i].state_topic, *relay_definitions[i].state_ref ? "ON" : "OFF", true);
  }
  mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
  mqtt_client.publish(MQTT_TOPIC_PROFILE_STATE, current_profile, true);
}
