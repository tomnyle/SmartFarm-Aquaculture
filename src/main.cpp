#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <math.h>
#include <string.h>
#include "app_config.h"
#include "pins.h"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterTemp(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

namespace {
constexpr float WATER_LEVEL_MIN = 20.0f;
constexpr float WATER_LEVEL_MAX = 100.0f;
constexpr float TURBIDITY_MAX = 5.0f;
constexpr float HUMIDITY_MIN = 50.0f;
constexpr float HUMIDITY_MAX = 80.0f;
constexpr float LIGHT_MIN = 500.0f;
constexpr uint32_t FEEDER_INTERVAL_MS = 12UL * 60UL * 60UL * 1000UL;
constexpr uint32_t FEEDER_DURATION_MS = 5UL * 1000UL;
constexpr uint32_t PUMP_SCHEDULE_INTERVAL_MS = 90UL * 60UL * 1000UL;
constexpr uint32_t PUMP_SCHEDULE_DURATION_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t CIRCULATION_INTERVAL_MS = 60UL * 60UL * 1000UL;
constexpr uint32_t CIRCULATION_DURATION_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t AERATOR_INTERVAL_MS = 30UL * 60UL * 1000UL;
constexpr uint32_t AERATOR_DURATION_MS = 10UL * 60UL * 1000UL;
}  // namespace

enum SensorStatus : uint8_t {
  NORMAL,
  WARNING,
  CRITICAL,
  UNAVAILABLE
};

struct SensorReading {
  float value;
  float min_threshold;
  float max_threshold;
  float critical_low;
  float critical_high;
  uint32_t timestamp;
  SensorStatus status;
  bool error;
  String error_msg;
};

struct SensorState {
  SensorReading water_temp;
  SensorReading ph;
  SensorReading dissolved_oxygen;
  SensorReading water_level;
  SensorReading turbidity;
  SensorReading air_temp;
  SensorReading humidity;
  SensorReading light;
  SensorReading co2;
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

struct ProfileConfig {
  const char* name;
  float temp_min;
  float temp_max;
  float ph_min;
  float ph_max;
  float do_min;
  float do_critical;
  float water_level_min;
  float water_level_max;
  float turbidity_max;
  float humidity_min;
  float humidity_max;
  float light_min;
  float co2_max;
};

struct SensorBinding {
  const char* key;
  const char* title;
  const char* unit;
  const char* icon;
  const char* device_class;
  uint8_t decimals;
  SensorReading* reading;
};

struct RelayBinding {
  const char* key;
  const char* title;
  const char* icon;
  uint8_t pin;
  bool* state;
};

static const ProfileConfig PROFILES[] = {
    {"Koi", 18.0f, 28.0f, 6.8f, 8.2f, 5.5f, 3.5f, WATER_LEVEL_MIN, WATER_LEVEL_MAX, 4.5f, HUMIDITY_MIN, HUMIDITY_MAX, LIGHT_MIN, 5.0f},
    {"Catfish", 20.0f, 32.0f, 6.5f, 8.5f, 4.5f, 3.0f, WATER_LEVEL_MIN, WATER_LEVEL_MAX, 5.0f, HUMIDITY_MIN, HUMIDITY_MAX, 300.0f, 6.0f},
    {"Shrimp", static_cast<float>(TEMP_ALERT_LOW), static_cast<float>(TEMP_ALERT_HIGH), static_cast<float>(PH_ALERT_LOW), static_cast<float>(PH_ALERT_HIGH), static_cast<float>(DO_ALERT_LOW), static_cast<float>(DO_CRITICAL_LOW), WATER_LEVEL_MIN, WATER_LEVEL_MAX, TURBIDITY_MAX, HUMIDITY_MIN, HUMIDITY_MAX, LIGHT_MIN, static_cast<float>(CO2_ALERT_HIGH)},
    {"Tilapia", 22.0f, 30.0f, 6.5f, 8.5f, 5.0f, 3.0f, WATER_LEVEL_MIN, WATER_LEVEL_MAX, 5.0f, HUMIDITY_MIN, HUMIDITY_MAX, 400.0f, 5.0f},
};

SensorState sensors;
OutputState outputs;

bool light_sensor_available = false;
uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;
char requested_mode[16] = DEFAULT_MODE;
char active_mode[16] = DEFAULT_MODE;
char current_profile[16] = "Shrimp";
char system_status[16] = "INIT";
char system_error[128] = "";

SensorBinding sensor_bindings[] = {
    {"water_temp", "Water Temp", "°C", "mdi:thermometer", "temperature", 2, &sensors.water_temp},
    {"ph", "pH", "pH", "mdi:test-tube", nullptr, 2, &sensors.ph},
    {"do", "Dissolved O₂", "mg/L", "mdi:water", nullptr, 2, &sensors.dissolved_oxygen},
    {"water_level", "Water Level", "%", "mdi:water-percent", nullptr, 1, &sensors.water_level},
    {"turbidity", "Turbidity", "NTU", "mdi:water-opacity", nullptr, 2, &sensors.turbidity},
    {"air_temp", "Air Temp", "°C", "mdi:home-thermometer", "temperature", 2, &sensors.air_temp},
    {"humidity", "Humidity", "%", "mdi:water-percent", "humidity", 1, &sensors.humidity},
    {"light", "Light", "lux", "mdi:brightness-6", "illuminance", 0, &sensors.light},
    {"co2", "CO2", "ppm", "mdi:molecule-co2", "carbon_dioxide", 2, &sensors.co2},
};

RelayBinding relay_bindings[] = {
    {"pump", "Pump", "mdi:pump", PUMP_PIN, &outputs.pump},
    {"aerator", "Aerator", "mdi:air-purifier", AERATOR_PIN, &outputs.aerator},
    {"circulation", "Circulation", "mdi:water-sync", CIRCULATION_PIN, &outputs.circulation},
    {"feeder", "Feeder", "mdi:fish-food", FEEDER_PIN, &outputs.feeder},
    {"spare1", "Spare 1", "mdi:toggle-switch", SPARE1_PIN, &outputs.spare1},
    {"spare2", "Spare 2", "mdi:toggle-switch-off", SPARE2_PIN, &outputs.spare2},
};

void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void update_sensor_statuses();
void apply_profile_thresholds();
void apply_rules();
void execute_schedule();
void check_critical_conditions();
void publish_mqtt_state();
void publish_sensor_data();
void publish_output_state();
void set_output(const char* name, bool state);
void apply_safe_mode_outputs();

String base_topic() {
  return String("smartfarm/aquaculture/") + FW_DEVICE_ID;
}

String sensor_topic(const char* name, const char* suffix = nullptr) {
  String topic = base_topic() + "/sensor/" + name;
  if (suffix && suffix[0] != '\0') {
    topic += "/";
    topic += suffix;
  }
  return topic;
}

String relay_state_topic(const char* name) {
  return base_topic() + "/relay/" + name + "/state";
}

String relay_command_topic(const char* name) {
  return base_topic() + "/relay/" + name + "/command/set";
}

String mode_topic() {
  return base_topic() + "/mode";
}

String mode_set_topic() {
  return base_topic() + "/mode/set";
}

String profile_topic() {
  return base_topic() + "/profile";
}

String profile_set_topic() {
  return base_topic() + "/profile/set";
}

String status_topic() {
  return base_topic() + "/status";
}

String error_topic() {
  return base_topic() + "/error";
}

String state_topic() {
  return base_topic() + "/state";
}

String availability_topic() {
  return base_topic() + "/availability";
}

String status_to_string(SensorStatus status) {
  switch (status) {
    case NORMAL:
      return "NORMAL";
    case WARNING:
      return "WARNING";
    case CRITICAL:
      return "CRITICAL";
    case UNAVAILABLE:
    default:
      return "UNAVAILABLE";
  }
}

bool is_on_message(const String& message) {
  return message.equalsIgnoreCase("ON") || message == "1" || message.equalsIgnoreCase("true");
}

bool is_valid_mode(const String& mode) {
  return mode == "AUTO" || mode == "MANUAL" || mode == "SCHEDULE" || mode == "SAFE";
}

const ProfileConfig* find_profile(const char* name) {
  for (const auto& profile : PROFILES) {
    if (strcmp(profile.name, name) == 0) {
      return &profile;
    }
  }
  return nullptr;
}

void initialize_sensor(SensorReading& reading) {
  reading.value = NAN;
  reading.min_threshold = NAN;
  reading.max_threshold = NAN;
  reading.critical_low = NAN;
  reading.critical_high = NAN;
  reading.timestamp = 0;
  reading.status = UNAVAILABLE;
  reading.error = true;
  reading.error_msg = "Not initialized";
}

void set_sensor_reading(SensorReading& reading, float value, bool valid, const String& error_msg = "") {
  reading.timestamp = millis();
  if (valid) {
    reading.value = value;
    reading.error = false;
    reading.error_msg = "";
  } else {
    reading.value = NAN;
    reading.error = true;
    reading.error_msg = error_msg;
  }
}

bool is_warning_near_min(float value, float minimum, float maximum) {
  const float range = (isnan(maximum) ? fabsf(minimum) : fabsf(maximum - minimum));
  const float margin = max(0.25f, range * 0.1f);
  return value <= minimum + margin;
}

bool is_warning_near_max(float value, float minimum, float maximum) {
  const float range = (isnan(minimum) ? fabsf(maximum) : fabsf(maximum - minimum));
  const float margin = max(0.25f, range * 0.1f);
  return value >= maximum - margin;
}

SensorStatus evaluate_sensor_status(const SensorReading& reading) {
  if (reading.error || isnan(reading.value)) {
    return UNAVAILABLE;
  }

  if (!isnan(reading.critical_low) && reading.value < reading.critical_low) {
    return CRITICAL;
  }
  if (!isnan(reading.critical_high) && reading.value > reading.critical_high) {
    return CRITICAL;
  }

  if (!isnan(reading.min_threshold) && !isnan(reading.max_threshold)) {
    if (reading.value < reading.min_threshold || reading.value > reading.max_threshold) {
      return CRITICAL;
    }
    if (is_warning_near_min(reading.value, reading.min_threshold, reading.max_threshold) ||
        is_warning_near_max(reading.value, reading.min_threshold, reading.max_threshold)) {
      return WARNING;
    }
    return NORMAL;
  }

  if (!isnan(reading.min_threshold)) {
    if (reading.value < reading.min_threshold) {
      return isnan(reading.critical_low) ? CRITICAL : WARNING;
    }
    if (is_warning_near_min(reading.value, reading.min_threshold, NAN)) {
      return WARNING;
    }
  }

  if (!isnan(reading.max_threshold)) {
    if (reading.value > reading.max_threshold) {
      return CRITICAL;
    }
    if (is_warning_near_max(reading.value, NAN, reading.max_threshold)) {
      return WARNING;
    }
  }

  return NORMAL;
}

void apply_profile_thresholds() {
  const ProfileConfig* profile = find_profile(current_profile);
  if (!profile) {
    profile = &PROFILES[2];
    strcpy(current_profile, profile->name);
  }

  sensors.water_temp.min_threshold = profile->temp_min;
  sensors.water_temp.max_threshold = profile->temp_max;
  sensors.water_temp.critical_low = profile->temp_min;
  sensors.water_temp.critical_high = profile->temp_max;

  sensors.ph.min_threshold = profile->ph_min;
  sensors.ph.max_threshold = profile->ph_max;
  sensors.ph.critical_low = profile->ph_min;
  sensors.ph.critical_high = profile->ph_max;

  sensors.dissolved_oxygen.min_threshold = profile->do_min;
  sensors.dissolved_oxygen.max_threshold = NAN;
  sensors.dissolved_oxygen.critical_low = profile->do_critical;
  sensors.dissolved_oxygen.critical_high = NAN;

  sensors.water_level.min_threshold = profile->water_level_min;
  sensors.water_level.max_threshold = profile->water_level_max;
  sensors.water_level.critical_low = profile->water_level_min;
  sensors.water_level.critical_high = profile->water_level_max;

  sensors.turbidity.min_threshold = NAN;
  sensors.turbidity.max_threshold = profile->turbidity_max;
  sensors.turbidity.critical_low = NAN;
  sensors.turbidity.critical_high = profile->turbidity_max;

  sensors.air_temp.min_threshold = NAN;
  sensors.air_temp.max_threshold = NAN;
  sensors.air_temp.critical_low = NAN;
  sensors.air_temp.critical_high = NAN;

  sensors.humidity.min_threshold = profile->humidity_min;
  sensors.humidity.max_threshold = profile->humidity_max;
  sensors.humidity.critical_low = profile->humidity_min;
  sensors.humidity.critical_high = profile->humidity_max;

  sensors.light.min_threshold = profile->light_min;
  sensors.light.max_threshold = NAN;
  sensors.light.critical_low = NAN;
  sensors.light.critical_high = NAN;

  sensors.co2.min_threshold = NAN;
  sensors.co2.max_threshold = profile->co2_max;
  sensors.co2.critical_low = NAN;
  sensors.co2.critical_high = profile->co2_max;
}

void update_sensor_statuses() {
  for (auto& sensor : sensor_bindings) {
    sensor.reading->status = evaluate_sensor_status(*sensor.reading);
  }
}

String format_float(float value, uint8_t decimals) {
  if (isnan(value)) {
    return "nan";
  }
  return String(value, decimals);
}

bool sensor_is_critical(const SensorReading& reading) {
  return reading.status == CRITICAL;
}

bool sensor_is_unavailable(const SensorReading& reading) {
  return reading.status == UNAVAILABLE;
}

void publish_numeric_if_valid(const String& topic, float value, uint8_t decimals) {
  if (!isnan(value)) {
    mqtt_client.publish(topic.c_str(), format_float(value, decimals).c_str(), true);
  }
}

void publish_discovery_payload(const String& config_topic, JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(config_topic.c_str(), payload.c_str(), true);
  delay(20);
}

void publish_sensor_discovery(const SensorBinding& binding) {
  StaticJsonDocument<768> doc;
  const String object_id = String(FW_DEVICE_ID) + "_" + binding.key;
  doc["name"] = String("Aquaculture ") + binding.title;
  doc["unique_id"] = object_id;
  doc["state_topic"] = sensor_topic(binding.key).c_str();
  doc["json_attributes_topic"] = sensor_topic(binding.key, "attributes").c_str();
  doc["availability_topic"] = availability_topic().c_str();
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["icon"] = binding.icon;
  if (binding.unit && binding.unit[0] != '\0') {
    doc["unit_of_measurement"] = binding.unit;
  }
  if (binding.device_class) {
    doc["device_class"] = binding.device_class;
  }
  doc["device"]["identifiers"][0] = FW_DEVICE_ID;
  doc["device"]["name"] = DEVICE_NAME;
  doc["device"]["manufacturer"] = "SmartFarm";
  doc["device"]["model"] = "ESP32 Aquaculture";
  doc["device"]["sw_version"] = FW_VERSION;

  publish_discovery_payload(String(HA_DISCOVERY_PREFIX) + "/sensor/" + object_id + "/config", doc);

  StaticJsonDocument<512> status_doc;
  const String status_object_id = object_id + "_status";
  status_doc["name"] = String("Aquaculture ") + binding.title + " Status";
  status_doc["unique_id"] = status_object_id;
  status_doc["state_topic"] = sensor_topic(binding.key, "status").c_str();
  status_doc["availability_topic"] = availability_topic().c_str();
  status_doc["payload_available"] = "online";
  status_doc["payload_not_available"] = "offline";
  status_doc["icon"] = "mdi:alert-circle-outline";
  status_doc["entity_category"] = "diagnostic";
  status_doc["device"]["identifiers"][0] = FW_DEVICE_ID;
  status_doc["device"]["name"] = DEVICE_NAME;
  publish_discovery_payload(String(HA_DISCOVERY_PREFIX) + "/sensor/" + status_object_id + "/config", status_doc);
}

void publish_switch_discovery(const RelayBinding& binding) {
  StaticJsonDocument<512> doc;
  const String object_id = String(FW_DEVICE_ID) + "_" + binding.key;
  doc["name"] = String("Aquaculture ") + binding.title;
  doc["unique_id"] = object_id;
  doc["state_topic"] = relay_state_topic(binding.key).c_str();
  doc["command_topic"] = relay_command_topic(binding.key).c_str();
  doc["availability_topic"] = availability_topic().c_str();
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["icon"] = binding.icon;
  doc["device"]["identifiers"][0] = FW_DEVICE_ID;
  doc["device"]["name"] = DEVICE_NAME;
  doc["device"]["manufacturer"] = "SmartFarm";
  doc["device"]["model"] = "ESP32 Aquaculture";
  doc["device"]["sw_version"] = FW_VERSION;
  publish_discovery_payload(String(HA_DISCOVERY_PREFIX) + "/switch/" + object_id + "/config", doc);
}

void publish_select_discovery(const char* key, const char* title, const char* state_topic_name,
                              const char* command_topic_name, const char* icon,
                              const char* const* options, size_t option_count) {
  StaticJsonDocument<768> doc;
  const String object_id = String(FW_DEVICE_ID) + "_" + key;
  doc["name"] = title;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic_name;
  doc["command_topic"] = command_topic_name;
  doc["availability_topic"] = availability_topic().c_str();
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["icon"] = icon;
  for (size_t i = 0; i < option_count; ++i) {
    doc["options"][i] = options[i];
  }
  doc["device"]["identifiers"][0] = FW_DEVICE_ID;
  doc["device"]["name"] = DEVICE_NAME;
  doc["device"]["manufacturer"] = "SmartFarm";
  doc["device"]["model"] = "ESP32 Aquaculture";
  doc["device"]["sw_version"] = FW_VERSION;
  publish_discovery_payload(String(HA_DISCOVERY_PREFIX) + "/select/" + object_id + "/config", doc);
}

void publish_text_sensor_discovery(const char* key, const char* title, const String& topic, const char* icon) {
  StaticJsonDocument<512> doc;
  const String object_id = String(FW_DEVICE_ID) + "_" + key;
  doc["name"] = title;
  doc["unique_id"] = object_id;
  doc["state_topic"] = topic.c_str();
  doc["availability_topic"] = availability_topic().c_str();
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["icon"] = icon;
  doc["entity_category"] = "diagnostic";
  doc["device"]["identifiers"][0] = FW_DEVICE_ID;
  doc["device"]["name"] = DEVICE_NAME;
  publish_discovery_payload(String(HA_DISCOVERY_PREFIX) + "/sensor/" + object_id + "/config", doc);
}

void publish_mqtt_discovery() {
  for (const auto& binding : sensor_bindings) {
    publish_sensor_discovery(binding);
  }

  for (const auto& relay : relay_bindings) {
    publish_switch_discovery(relay);
  }

  static const char* const mode_options[] = {"AUTO", "MANUAL", "SCHEDULE", "SAFE"};
  publish_select_discovery("mode", "Aquaculture Mode", mode_topic().c_str(), mode_set_topic().c_str(),
                           "mdi:tune-variant", mode_options, 4);

  static const char* const profile_options[] = {"Koi", "Catfish", "Shrimp", "Tilapia"};
  publish_select_discovery("profile", "Aquaculture Profile", profile_topic().c_str(),
                           profile_set_topic().c_str(), "mdi:fish", profile_options, 4);

  publish_text_sensor_discovery("controller_status", "Aquaculture Status", status_topic(), "mdi:shield-check");
  publish_text_sensor_discovery("controller_error", "Aquaculture Error", error_topic(), "mdi:alert-octagon");
}

void publish_sensor_attributes(const SensorBinding& binding) {
  StaticJsonDocument<256> doc;
  doc["status"] = status_to_string(binding.reading->status);
  doc["error"] = binding.reading->error;
  if (!binding.reading->error_msg.isEmpty()) {
    doc["error_message"] = binding.reading->error_msg;
  }
  if (!isnan(binding.reading->min_threshold)) {
    doc["min"] = binding.reading->min_threshold;
  }
  if (!isnan(binding.reading->max_threshold)) {
    doc["max"] = binding.reading->max_threshold;
  }
  if (!isnan(binding.reading->critical_low)) {
    doc["critical_low"] = binding.reading->critical_low;
  }
  if (!isnan(binding.reading->critical_high)) {
    doc["critical_high"] = binding.reading->critical_high;
  }
  doc["profile"] = current_profile;
  doc["updated_ms"] = binding.reading->timestamp;

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(sensor_topic(binding.key, "attributes").c_str(), payload.c_str(), true);
}

void publish_sensor_data() {
  if (!mqtt_client.connected()) {
    return;
  }

  for (const auto& binding : sensor_bindings) {
    publish_numeric_if_valid(sensor_topic(binding.key), binding.reading->value, binding.decimals);
    mqtt_client.publish(sensor_topic(binding.key, "status").c_str(), status_to_string(binding.reading->status).c_str(), true);
    publish_sensor_attributes(binding);

    publish_numeric_if_valid(sensor_topic(binding.key, "min"), binding.reading->min_threshold, binding.decimals);
    publish_numeric_if_valid(sensor_topic(binding.key, "max"), binding.reading->max_threshold, binding.decimals);
    publish_numeric_if_valid(sensor_topic(binding.key, "critical"), binding.reading->critical_low, binding.decimals);
  }
}

void publish_output_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  for (const auto& relay : relay_bindings) {
    mqtt_client.publish(relay_state_topic(relay.key).c_str(), *relay.state ? "ON" : "OFF", true);
  }

  mqtt_client.publish(mode_topic().c_str(), active_mode, true);
  mqtt_client.publish(profile_topic().c_str(), current_profile, true);
  mqtt_client.publish(status_topic().c_str(), system_status, true);
  mqtt_client.publish(error_topic().c_str(), system_error[0] == '\0' ? "OK" : system_error, true);
  mqtt_client.publish(availability_topic().c_str(), "online", true);
}

void publish_mqtt_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  publish_sensor_data();
  publish_output_state();

  StaticJsonDocument<2048> doc;
  doc["device_id"] = FW_DEVICE_ID;
  doc["mode"] = active_mode;
  doc["requested_mode"] = requested_mode;
  doc["profile"] = current_profile;
  doc["status"] = system_status;
  doc["error"] = system_error[0] == '\0' ? "OK" : system_error;
  doc["uptime_ms"] = millis();
  doc["last_update_ms"] = sensors.last_read;

  JsonObject sensor_json = doc["sensors"].to<JsonObject>();
  for (const auto& binding : sensor_bindings) {
    JsonObject item = sensor_json[binding.key].to<JsonObject>();
    if (!isnan(binding.reading->value)) {
      item["value"] = binding.reading->value;
    }
    item["status"] = status_to_string(binding.reading->status);
  }

  JsonObject relay_json = doc["relays"].to<JsonObject>();
  for (const auto& relay : relay_bindings) {
    relay_json[relay.key] = *relay.state ? "ON" : "OFF";
  }

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(state_topic().c_str(), payload.c_str(), true);
}

void set_output(const char* name, bool state) {
  for (auto& relay : relay_bindings) {
    if (strcmp(relay.key, name) == 0) {
      if (*relay.state == state) {
        return;
      }
      *relay.state = state;
      digitalWrite(relay.pin, state ? HIGH : LOW);
      Serial.print("[OUTPUT] ");
      Serial.print(relay.title);
      Serial.println(state ? " ON" : " OFF");
      return;
    }
  }
}

void apply_safe_mode_outputs() {
  set_output("pump", true);
  set_output("aerator", true);
  set_output("circulation", true);
  set_output("feeder", false);
  set_output("spare1", false);
  set_output("spare2", false);
}

void apply_rules() {
  bool pump_on = false;
  bool aerator_on = false;
  bool circulation_on = false;

  if (!sensor_is_unavailable(sensors.water_temp) && !isnan(sensors.water_temp.value) &&
      sensors.water_temp.value >= AUTO_CIRCULATION_TEMP_HIGH) {
    circulation_on = true;
  }

  if (!sensor_is_unavailable(sensors.water_temp) && !isnan(sensors.water_temp.value) &&
      sensors.water_temp.value >= AUTO_PUMP_TEMP_HIGH) {
    pump_on = true;
  }

  if (sensors.dissolved_oxygen.status == WARNING || sensors.dissolved_oxygen.status == CRITICAL ||
      (!isnan(sensors.dissolved_oxygen.value) && sensors.dissolved_oxygen.value < AUTO_AERATOR_DO_LOW)) {
    aerator_on = true;
  }

  if (sensors.dissolved_oxygen.status == WARNING || sensors.dissolved_oxygen.status == CRITICAL ||
      (!isnan(sensors.dissolved_oxygen.value) && sensors.dissolved_oxygen.value < AUTO_PUMP_DO_LOW)) {
    pump_on = true;
  }

  if (sensors.co2.status == WARNING || sensors.co2.status == CRITICAL) {
    pump_on = true;
    aerator_on = true;
  }

  if (sensors.turbidity.status == WARNING || sensors.turbidity.status == CRITICAL) {
    circulation_on = true;
  }

  if (sensors.water_level.status == WARNING || sensors.water_level.status == CRITICAL) {
    pump_on = false;
  }

  set_output("pump", pump_on);
  set_output("aerator", aerator_on);
  set_output("circulation", circulation_on);
  set_output("feeder", false);
}

bool window_active(uint32_t now, uint32_t interval, uint32_t duration) {
  return duration > 0 && interval > 0 && (now % interval) < duration;
}

void execute_schedule() {
  const uint32_t now = millis();
  set_output("feeder", window_active(now, FEEDER_INTERVAL_MS, FEEDER_DURATION_MS));
  set_output("pump", sensors.water_level.status == CRITICAL ? false : window_active(now, PUMP_SCHEDULE_INTERVAL_MS, PUMP_SCHEDULE_DURATION_MS));
  set_output("circulation", window_active(now, CIRCULATION_INTERVAL_MS, CIRCULATION_DURATION_MS));
  set_output("aerator", window_active(now, AERATOR_INTERVAL_MS, AERATOR_DURATION_MS) || sensors.dissolved_oxygen.status == WARNING || sensors.dissolved_oxygen.status == CRITICAL);
}

void check_critical_conditions() {
  system_error[0] = '\0';

  if (sensor_is_unavailable(sensors.water_temp) || sensor_is_unavailable(sensors.ph) ||
      sensor_is_unavailable(sensors.dissolved_oxygen) || sensor_is_unavailable(sensors.water_level)) {
    strncpy(system_status, "SAFE", sizeof(system_status));
    strncpy(active_mode, "SAFE", sizeof(active_mode));
    strncpy(system_error, "Essential sensor unavailable", sizeof(system_error) - 1);
    apply_safe_mode_outputs();
    return;
  }

  if (sensor_is_critical(sensors.water_temp)) {
    strncpy(system_status, "SAFE", sizeof(system_status));
    strncpy(active_mode, "SAFE", sizeof(active_mode));
    strncpy(system_error, "Water temperature critical", sizeof(system_error) - 1);
    apply_safe_mode_outputs();
    return;
  }

  if (sensor_is_critical(sensors.dissolved_oxygen)) {
    strncpy(system_status, "SAFE", sizeof(system_status));
    strncpy(active_mode, "SAFE", sizeof(active_mode));
    strncpy(system_error, "Dissolved oxygen critical", sizeof(system_error) - 1);
    apply_safe_mode_outputs();
    return;
  }

  if (sensor_is_critical(sensors.water_level)) {
    strncpy(system_status, "SAFE", sizeof(system_status));
    strncpy(active_mode, "SAFE", sizeof(active_mode));
    strncpy(system_error, "Water level critical", sizeof(system_error) - 1);
    apply_safe_mode_outputs();
    return;
  }

  if (sensor_is_critical(sensors.co2)) {
    strncpy(system_status, "SAFE", sizeof(system_status));
    strncpy(active_mode, "SAFE", sizeof(active_mode));
    strncpy(system_error, "CO2 critical", sizeof(system_error) - 1);
    apply_safe_mode_outputs();
    return;
  }

  strncpy(active_mode, requested_mode, sizeof(active_mode) - 1);
  active_mode[sizeof(active_mode) - 1] = '\0';

  if (strcmp(active_mode, "SAFE") == 0) {
    strncpy(system_status, "SAFE", sizeof(system_status));
    strncpy(system_error, "SAFE mode requested", sizeof(system_error) - 1);
    apply_safe_mode_outputs();
    return;
  }

  if (mqtt_client.connected()) {
    strncpy(system_status, "RUNNING", sizeof(system_status));
  } else {
    strncpy(system_status, "ERROR", sizeof(system_status));
    strncpy(system_error, "MQTT disconnected", sizeof(system_error) - 1);
  }
}

void read_sensors() {
  waterTemp.requestTemperatures();
  const float water_temperature = waterTemp.getTempCByIndex(0);
  set_sensor_reading(sensors.water_temp, water_temperature, water_temperature > -55.0f && water_temperature < 125.0f,
                     "DS18B20 unavailable");

  const float ph_value = (analogRead(PH_PIN) / 4095.0f) * 14.0f;
  set_sensor_reading(sensors.ph, ph_value, true);

  const float do_value = (analogRead(DO_PIN) / 4095.0f) * 20.0f;
  set_sensor_reading(sensors.dissolved_oxygen, do_value, true);

  const float water_level = (analogRead(WATER_LEVEL_PIN) / 4095.0f) * 100.0f;
  set_sensor_reading(sensors.water_level, water_level, true);

  const float turbidity = (analogRead(TURBIDITY_PIN) / 4095.0f) * 10.0f;
  set_sensor_reading(sensors.turbidity, turbidity, true);

  const float air_temp = dht.readTemperature();
  set_sensor_reading(sensors.air_temp, air_temp, !isnan(air_temp), "DHT22 temperature unavailable");

  const float humidity = dht.readHumidity();
  set_sensor_reading(sensors.humidity, humidity, !isnan(humidity), "DHT22 humidity unavailable");

  const float light = light_sensor_available ? lightMeter.readLightLevel() : NAN;
  set_sensor_reading(sensors.light, light, light_sensor_available && !isnan(light) && light >= 0.0f,
                     "BH1750 unavailable");

  const float co2 = (analogRead(CO2_PIN) / 4095.0f) * 10.0f;
  set_sensor_reading(sensors.co2, co2, true);

  sensors.last_read = millis();
  update_sensor_statuses();

  Serial.println("===== SENSOR READINGS =====");
  for (const auto& binding : sensor_bindings) {
    Serial.print(binding.title);
    Serial.print(": ");
    Serial.print(format_float(binding.reading->value, binding.decimals));
    if (binding.unit && binding.unit[0] != '\0') {
      Serial.print(' ');
      Serial.print(binding.unit);
    }
    Serial.print(" [");
    Serial.print(status_to_string(binding.reading->status));
    Serial.println("]");
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }
  message.trim();

  const String topic_name(topic);
  Serial.print("[MQTT] ");
  Serial.print(topic_name);
  Serial.print(" => ");
  Serial.println(message);

  if (topic_name == mode_set_topic()) {
    if (is_valid_mode(message)) {
      strncpy(requested_mode, message.c_str(), sizeof(requested_mode) - 1);
      requested_mode[sizeof(requested_mode) - 1] = '\0';
    }
    return;
  }

  if (topic_name == profile_set_topic()) {
    if (find_profile(message.c_str())) {
      strncpy(current_profile, message.c_str(), sizeof(current_profile) - 1);
      current_profile[sizeof(current_profile) - 1] = '\0';
      apply_profile_thresholds();
      update_sensor_statuses();
    }
    return;
  }

  for (const auto& relay : relay_bindings) {
    if (topic_name == relay_command_topic(relay.key)) {
      strncpy(requested_mode, "MANUAL", sizeof(requested_mode) - 1);
      requested_mode[sizeof(requested_mode) - 1] = '\0';
      if (strcmp(active_mode, "SAFE") != 0) {
        set_output(relay.key, is_on_message(message));
      }
      return;
    }
  }
}

void setup_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("[WiFi] Connection timeout");
  }
}

void reconnect_mqtt() {
  if (mqtt_client.connected()) {
    return;
  }

  static uint32_t last_reconnect_attempt = 0;
  const uint32_t now = millis();
  if (now - last_reconnect_attempt < MQTT_RECONNECT_INTERVAL) {
    return;
  }
  last_reconnect_attempt = now;

  const String will_topic = availability_topic();
  Serial.print("[MQTT] Connecting to ");
  Serial.println(MQTT_BROKER);

  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD, will_topic.c_str(), 0, true, "offline")) {
    Serial.println("[MQTT] Connected");
    mqtt_client.publish(availability_topic().c_str(), "online", true);
    mqtt_client.subscribe(mode_set_topic().c_str());
    mqtt_client.subscribe(profile_set_topic().c_str());
    for (const auto& relay : relay_bindings) {
      mqtt_client.subscribe(relay_command_topic(relay.key).c_str());
    }
    publish_mqtt_discovery();
    publish_output_state();
  } else {
    Serial.print("[MQTT] Connect failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n===== SmartFarm Aquaculture Controller =====");

  analogReadResolution(12);
  apply_profile_thresholds();
  for (auto& sensor : sensor_bindings) {
    initialize_sensor(*sensor.reading);
  }
  apply_profile_thresholds();

  for (const auto& relay : relay_bindings) {
    pinMode(relay.pin, OUTPUT);
    digitalWrite(relay.pin, LOW);
    *relay.state = false;
  }

  waterTemp.begin();
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL);
  light_sensor_available = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  setup_wifi();
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(2048);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!mqtt_client.connected()) {
    reconnect_mqtt();
  } else {
    mqtt_client.loop();
  }

  const uint32_t now = millis();

  if (now - last_sensor_read >= SENSOR_READ_INTERVAL) {
    read_sensors();
    last_sensor_read = now;
  }

  check_critical_conditions();

  if (strcmp(active_mode, "AUTO") == 0 && now - last_rule_engine >= RULE_ENGINE_INTERVAL) {
    apply_rules();
    last_rule_engine = now;
  } else if (strcmp(active_mode, "SCHEDULE") == 0 && now - last_rule_engine >= RULE_ENGINE_INTERVAL) {
    execute_schedule();
    last_rule_engine = now;
  } else if (strcmp(active_mode, "SAFE") == 0) {
    apply_safe_mode_outputs();
  }

  if (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL) {
    publish_mqtt_state();
    last_mqtt_publish = now;
  }

  delay(10);
}
