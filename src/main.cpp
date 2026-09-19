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
#include "species_rules.h"
#include "aquaculture_logic.h"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterTemp(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

SensorData sensors = {};
OutputState outputs = {};
OutputState manual_outputs = {};
ControlState control_state;

RelayTracker pump_tracker = {};
RelayTracker aerator1_tracker = {};
RelayTracker aerator2_tracker = {};
RelayTracker circulation_tracker = {};
RelayTracker feeder_tracker = {};
RelayTracker alarm_tracker = {};

uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;
char current_species[32] = "Rô Phi";
char last_published_event_key[64] = "";

void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void evaluate_control_logic();
void publish_sensor_data();
void publish_output_state();
void publish_safety_state();
void publish_heartbeat();
void publish_controller_state();
void publish_event_if_needed(const char* topic);
void set_output(const char* name, bool state, bool force = false);
void set_manual_output(const char* name, bool state);
void initialize_outputs();

float minf(float a, float b) { return a < b ? a : b; }
float maxf(float a, float b) { return a > b ? a : b; }

bool stringToBool(const String& value) {
  return value == "ON" || value == "on" || value == "1" || value == "true" || value == "TRUE";
}

bool isSupportedSpecies(const char* species) {
  return strcmp(species, "Cá Chép") == 0 || strcmp(species, "Rô Phi") == 0 || strcmp(species, "Cá Tra") == 0 || strcmp(species, "Tôm Thẻ") == 0;
}

template <typename TDoc>
void append_device_info(TDoc& doc) {
  doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
  doc["device"]["name"] = DEVICE_NAME;
  doc["device"]["model"] = "SmartAquaculture ESP32 V1";
  doc["device"]["manufacturer"] = "SmartFarm";
  doc["device"]["sw_version"] = FW_VERSION;
  doc["availability_topic"] = MQTT_TOPIC_STATUS;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
}

void publish_discovery_payload(const char* component, const char* object_id, JsonDocument& doc) {
  if (!mqtt_client.connected()) {
    return;
  }

  String topic = String(HA_DISCOVERY_PREFIX) + "/" + component + "/" + object_id + "/config";
  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(topic.c_str(), payload.c_str(), true);
}

void publish_sensor_discovery(const char* object_id, const char* name, const char* state_topic,
                              const char* unit, const char* icon, const char* device_class = nullptr) {
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  if (unit != nullptr && strlen(unit) > 0) {
    doc["unit_of_measurement"] = unit;
  }
  if (icon != nullptr && strlen(icon) > 0) {
    doc["icon"] = icon;
  }
  if (device_class != nullptr && strlen(device_class) > 0) {
    doc["device_class"] = device_class;
  }
  append_device_info(doc);
  publish_discovery_payload("sensor", object_id, doc);
}

void publish_switch_discovery(const char* object_id, const char* name, const char* state_topic,
                              const char* command_topic, const char* icon) {
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["icon"] = icon;
  append_device_info(doc);
  publish_discovery_payload("switch", object_id, doc);
}

void publish_binary_sensor_discovery(const char* object_id, const char* name, const char* state_topic,
                                     const char* icon) {
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["icon"] = icon;
  append_device_info(doc);
  publish_discovery_payload("binary_sensor", object_id, doc);
}

void publish_select_discovery(const char* object_id, const char* name, const char* state_topic,
                              const char* command_topic, const char* icon,
                              const char* const* options, size_t option_count) {
  StaticJsonDocument<1024> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["icon"] = icon;
  for (size_t i = 0; i < option_count; ++i) {
    doc["options"][i] = options[i];
  }
  append_device_info(doc);
  publish_discovery_payload("select", object_id, doc);
}

void copy_text(char* destination, size_t size, const char* source) {
  if (size == 0) {
    return;
  }
  strncpy(destination, source, size - 1);
  destination[size - 1] = '\0';
}

void mark_alarm_state(SafetyLevel level, const char* message, const char* event_key, bool requires_ack) {
  control_state.safety_level = level;
  control_state.alarm_active = level != SafetyLevel::NORMAL;
  control_state.emergency_active = level == SafetyLevel::EMERGENCY;
  control_state.safety_active = level == SafetyLevel::LOW || level == SafetyLevel::CRITICAL ||
                                level == SafetyLevel::EMERGENCY || level == SafetyLevel::SENSOR_FAULT;
  control_state.requires_ack = requires_ack;
  copy_text(control_state.alarm_text, sizeof(control_state.alarm_text), message);
  copy_text(control_state.event_key, sizeof(control_state.event_key), event_key);
}

int safety_rank(SafetyLevel level) {
  switch (level) {
    case SafetyLevel::NORMAL: return 0;
    case SafetyLevel::WARNING: return 1;
    case SafetyLevel::LOW: return 2;
    case SafetyLevel::CRITICAL: return 3;
    case SafetyLevel::EMERGENCY: return 4;
    case SafetyLevel::SENSOR_FAULT: return 5;
  }
  return 0;
}

void escalate_alarm(SafetyLevel& current_level, SafetyLevel candidate_level,
                    char* message, size_t message_size,
                    char* event_key, size_t event_key_size,
                    const char* candidate_message, const char* candidate_event) {
  if (safety_rank(candidate_level) >= safety_rank(current_level)) {
    current_level = candidate_level;
    copy_text(message, message_size, candidate_message);
    copy_text(event_key, event_key_size, candidate_event);
  }
}

void apply_safety_outputs(OutputState& desired, SafetyLevel level) {
  if (level == SafetyLevel::WARNING || level == SafetyLevel::LOW) {
    desired.aerator1 = true;
    desired.alarm = true;
  }

  if (level == SafetyLevel::CRITICAL || level == SafetyLevel::EMERGENCY) {
    desired.aerator1 = true;
    desired.aerator2 = true;
    desired.alarm = true;
    desired.feeder = false;
    desired.feeder_locked = true;
  }

  if (level == SafetyLevel::SENSOR_FAULT) {
    desired.aerator1 = true;
    desired.alarm = true;
    desired.feeder = false;
    desired.feeder_locked = true;
  }
}

float read_scaled_value(uint8_t pin, float max_value) {
  int raw = analogRead(pin);
  return (static_cast<float>(raw) / 4095.0f) * max_value;
}

float read_current_value(uint8_t pin) {
  return read_scaled_value(pin, CURRENT_SENSOR_FULL_SCALE_A);
}

void update_stale_flags(uint32_t now) {
  sensors.water_temp_stale = sensors.water_temp_last_valid > 0 && (now - sensors.water_temp_last_valid > SENSOR_STALE_TIMEOUT_MS);
  sensors.air_stale = sensors.air_last_valid > 0 && (now - sensors.air_last_valid > SENSOR_STALE_TIMEOUT_MS);
  sensors.ph_stale = sensors.ph_last_valid > 0 && (now - sensors.ph_last_valid > SENSOR_STALE_TIMEOUT_MS);
  sensors.do_stale = sensors.do_last_valid > 0 && (now - sensors.do_last_valid > SENSOR_STALE_TIMEOUT_MS);
  sensors.water_level_stale = sensors.water_level_last_valid > 0 && (now - sensors.water_level_last_valid > SENSOR_STALE_TIMEOUT_MS);
  sensors.aerator_current_stale = sensors.aerator_current_last_valid > 0 && (now - sensors.aerator_current_last_valid > SENSOR_STALE_TIMEOUT_MS);
  sensors.pump_current_stale = sensors.pump_current_last_valid > 0 && (now - sensors.pump_current_last_valid > SENSOR_STALE_TIMEOUT_MS);
}

bool current_out_of_range(float current) {
  return current < CURRENT_MIN_RUNNING_A || current > CURRENT_MAX_RUNNING_A;
}

bool evaluate_running_current(bool output_running, bool current_valid, float current_value, uint32_t& fault_since) {
  uint32_t now = millis();

  if (!output_running || !current_valid) {
    fault_since = 0;
    return false;
  }

  if (!current_out_of_range(current_value)) {
    fault_since = 0;
    return false;
  }

  if (fault_since == 0) {
    fault_since = now;
    return false;
  }

  return now - fault_since >= CURRENT_FAULT_CONFIRM_MS;
}

void initialize_outputs() {
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(AERATOR_1_PIN, OUTPUT);
  pinMode(AERATOR_2_PIN, OUTPUT);
  pinMode(CIRCULATION_PIN, OUTPUT);
  pinMode(FEEDER_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  clearOutputState(outputs);
  clearOutputState(manual_outputs);

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(AERATOR_1_PIN, LOW);
  digitalWrite(AERATOR_2_PIN, LOW);
  digitalWrite(CIRCULATION_PIN, LOW);
  digitalWrite(FEEDER_PIN, LOW);
  digitalWrite(ALARM_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");

  initializeControlState(control_state, DEFAULT_MODE);
  initialize_outputs();

  pinMode(PH_PIN, INPUT);
  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(DO_PIN, INPUT);
  pinMode(CO2_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(AERATOR_CURRENT_PIN, INPUT);
  pinMode(PUMP_CURRENT_PIN, INPUT);

  analogReadResolution(12);

  waterTemp.begin();
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL);

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[OK] BH1750 Light Sensor initialized");
  } else {
    Serial.println("[WARN] BH1750 Light Sensor not found (optional)");
  }

  setup_wifi();

  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(1536);

  read_sensors();
  evaluate_control_logic();

  Serial.println("===== Initialization Complete =====\n");
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

  uint32_t now = millis();

  if (control_state.requested_mode == ControlMode::MANUAL &&
      control_state.manual_override_since > 0 &&
      now - control_state.manual_override_since >= MANUAL_OVERRIDE_TIMEOUT_MS &&
      !control_state.safety_active) {
    control_state.requested_mode = ControlMode::AUTO;
    Serial.println("[MODE] Manual override timed out, returning to AUTO");
  }

  if (now - last_sensor_read >= SENSOR_READ_INTERVAL) {
    read_sensors();
    last_sensor_read = now;
  }

  if (now - last_rule_engine >= RULE_ENGINE_INTERVAL) {
    evaluate_control_logic();
    last_rule_engine = now;
  }

  if (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL) {
    if (mqtt_client.connected()) {
      publish_sensor_data();
      publish_output_state();
      publish_safety_state();
      publish_heartbeat();
      publish_controller_state();
      publish_event_if_needed(
        strcmp(control_state.event_key, "sensor_fault") == 0 ? MQTT_TOPIC_EVENT_SENSOR_FAULT :
        (strcmp(control_state.event_key, "device_fault") == 0 ? MQTT_TOPIC_EVENT_DEVICE_FAULT : MQTT_TOPIC_EVENT_ALARM)
      );
      Serial.println("[MQTT] Data published successfully");
    }
    last_mqtt_publish = now;
  }

  digitalWrite(LED_PIN, outputs.alarm ? HIGH : LOW);
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
    Serial.println("[WARN] WiFi connection timeout, local control continues offline");
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

  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_STATUS, 0, true, "offline")) {
    Serial.println("[OK] MQTT Connected!");
    mqtt_client.publish(MQTT_TOPIC_STATUS, "online", true);

    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_PUMP);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR_1);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR_2);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_CIRCULATION);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_FEEDER);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_ALARM);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_MODE);
    mqtt_client.subscribe(MQTT_TOPIC_CONFIG_SPECIES);

    publish_mqtt_discovery();
    publish_sensor_data();
    publish_output_state();
    publish_safety_state();
    publish_heartbeat();
    publish_controller_state();
  } else {
    Serial.print("[WARN] MQTT connection failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

void publish_mqtt_discovery() {
  if (!HA_DISCOVERY_ENABLED) {
    return;
  }

  const char* const mode_options[] = {"AUTO", "MANUAL", "SCHEDULE", "SAFE", "EMERGENCY"};
  const char* const species_options[] = {"Cá Chép", "Rô Phi", "Cá Tra", "Tôm Thẻ"};

  publish_sensor_discovery("aquaculture_water_temp", "Aquaculture Water Temperature", MQTT_TOPIC_WATER_TEMP, "°C", "mdi:thermometer", "temperature");
  publish_sensor_discovery("aquaculture_ph", "Aquaculture pH", MQTT_TOPIC_PH, "pH", "mdi:test-tube");
  publish_sensor_discovery("aquaculture_do", "Aquaculture Dissolved Oxygen", MQTT_TOPIC_DO, "mg/L", "mdi:waveform");
  publish_sensor_discovery("aquaculture_water_level", "Aquaculture Water Level", MQTT_TOPIC_WATER_LEVEL, "%", "mdi:water-percent");
  publish_sensor_discovery("aquaculture_aerator_current", "Aquaculture Aerator Current", MQTT_TOPIC_AERATOR_CURRENT, "A", "mdi:current-ac");
  publish_sensor_discovery("aquaculture_pump_current", "Aquaculture Pump Current", MQTT_TOPIC_PUMP_CURRENT, "A", "mdi:current-ac");
  publish_sensor_discovery("aquaculture_safety_state", "Aquaculture Safety State", MQTT_TOPIC_STATUS_SAFETY_STATE, "", "mdi:shield-alert");
  publish_sensor_discovery("aquaculture_alarm_text", "Aquaculture Alarm Text", MQTT_TOPIC_STATUS_ALARM_TEXT, "", "mdi:alarm-light");

  publish_switch_discovery("aquaculture_aerator_1", "Aquaculture Aerator 1", MQTT_TOPIC_AERATOR_1, MQTT_TOPIC_CONTROL_AERATOR_1, "mdi:air-filter");
  publish_switch_discovery("aquaculture_aerator_2", "Aquaculture Aerator 2", MQTT_TOPIC_AERATOR_2, MQTT_TOPIC_CONTROL_AERATOR_2, "mdi:air-filter");
  publish_switch_discovery("aquaculture_pump", "Aquaculture Pump", MQTT_TOPIC_PUMP, MQTT_TOPIC_CONTROL_PUMP, "mdi:pump");
  publish_switch_discovery("aquaculture_feeder", "Aquaculture Feeder", MQTT_TOPIC_FEEDER, MQTT_TOPIC_CONTROL_FEEDER, "mdi:fish-food");
  publish_switch_discovery("aquaculture_alarm", "Aquaculture Alarm", MQTT_TOPIC_ALARM_OUTPUT, MQTT_TOPIC_CONTROL_ALARM, "mdi:alarm-bell");

  publish_binary_sensor_discovery("aquaculture_alarm_active", "Aquaculture Alarm Active", MQTT_TOPIC_STATUS_ALARM_ACTIVE, "mdi:alarm-light");
  publish_binary_sensor_discovery("aquaculture_safety_active", "Aquaculture Safety Active", MQTT_TOPIC_STATUS_SAFETY_ACTIVE, "mdi:shield-check");
  publish_binary_sensor_discovery("aquaculture_emergency_active", "Aquaculture Emergency Active", MQTT_TOPIC_STATUS_EMERGENCY_ACTIVE, "mdi:alarm-light-outline");

  publish_select_discovery("aquaculture_mode", "Aquaculture Mode", MQTT_TOPIC_MODE_STATE, MQTT_TOPIC_CONTROL_MODE, "mdi:cog", mode_options, 5);
  publish_select_discovery("aquaculture_species", "Aquaculture Species", MQTT_TOPIC_SPECIES_STATE, MQTT_TOPIC_CONFIG_SPECIES, "mdi:fish", species_options, 4);

  Serial.println("[HA Discovery] V1 entities published");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }

  Serial.print("[MQTT] Message received on: ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(message);

  if (strcmp(topic, MQTT_TOPIC_CONTROL_MODE) == 0) {
    control_state.requested_mode = parseControlMode(message.c_str());
    if (control_state.requested_mode == ControlMode::MANUAL) {
      control_state.manual_override_since = millis();
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONFIG_SPECIES) == 0) {
    if (isSupportedSpecies(message.c_str())) {
      copy_text(current_species, sizeof(current_species), message.c_str());
    } else {
      Serial.println("[WARN] Unsupported species requested, keeping current profile");
    }
    return;
  }

  bool desired_state = stringToBool(message);

  if (strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0) {
    set_manual_output("pump", desired_state);
  } else if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR) == 0 || strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR_1) == 0) {
    set_manual_output("aerator1", desired_state);
  } else if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR_2) == 0) {
    set_manual_output("aerator2", desired_state);
  } else if (strcmp(topic, MQTT_TOPIC_CONTROL_CIRCULATION) == 0) {
    set_manual_output("circulation", desired_state);
  } else if (strcmp(topic, MQTT_TOPIC_CONTROL_FEEDER) == 0) {
    set_manual_output("feeder", desired_state);
  } else if (strcmp(topic, MQTT_TOPIC_CONTROL_ALARM) == 0) {
    set_manual_output("alarm", desired_state);
  }
}

void set_manual_output(const char* name, bool state) {
  control_state.requested_mode = ControlMode::MANUAL;
  control_state.manual_override_since = millis();

  if (strcmp(name, "pump") == 0) {
    manual_outputs.pump = state;
  } else if (strcmp(name, "aerator1") == 0) {
    manual_outputs.aerator1 = state;
  } else if (strcmp(name, "aerator2") == 0) {
    manual_outputs.aerator2 = state;
  } else if (strcmp(name, "circulation") == 0) {
    manual_outputs.circulation = state;
  } else if (strcmp(name, "feeder") == 0) {
    manual_outputs.feeder = state;
  } else if (strcmp(name, "alarm") == 0) {
    manual_outputs.alarm = state;
  }
}

void read_sensors() {
  uint32_t now = millis();

  waterTemp.requestTemperatures();
  float water_temp_reading = waterTemp.getTempCByIndex(0);
  if (water_temp_reading > -20.0f && water_temp_reading < 60.0f) {
    sensors.water_temp = water_temp_reading;
    sensors.water_temp_valid = true;
    sensors.water_temp_last_valid = now;
  } else {
    sensors.water_temp_valid = false;
  }

  float air_temp_reading = dht.readTemperature();
  float humidity_reading = dht.readHumidity();
  if (!isnan(air_temp_reading) && !isnan(humidity_reading) && humidity_reading >= 0.0f && humidity_reading <= 100.0f) {
    sensors.air_temp = air_temp_reading;
    sensors.air_humidity = humidity_reading;
    sensors.air_valid = true;
    sensors.air_last_valid = now;
  } else {
    sensors.air_valid = false;
  }

  float light_reading = lightMeter.readLightLevel();
  if (light_reading >= 0.0f) {
    sensors.light = light_reading;
    sensors.light_valid = true;
  } else {
    sensors.light_valid = false;
  }

  float previous_ph = sensors.ph;
  uint32_t previous_ph_time = sensors.ph_last_valid;
  float ph_reading = read_scaled_value(PH_PIN, 14.0f);
  if (ph_reading >= 0.0f && ph_reading <= 14.0f) {
    sensors.ph = ph_reading;
    sensors.ph_valid = true;
    sensors.ph_last_valid = now;
    if (previous_ph_time > 0 && now > previous_ph_time) {
      float hours = static_cast<float>(now - previous_ph_time) / 3600000.0f;
      if (hours > 0.0f) {
        sensors.ph_rate = fabs(ph_reading - previous_ph) / hours;
      }
    }
  } else {
    sensors.ph_valid = false;
  }

  float turbidity_reading = static_cast<float>(analogRead(TURBIDITY_PIN));
  sensors.turbidity = turbidity_reading;
  sensors.turbidity_valid = turbidity_reading >= 0.0f;

  float do_reading = read_scaled_value(DO_PIN, 20.0f);
  if (do_reading >= 0.0f && do_reading <= 20.0f) {
    sensors.do_value = do_reading;
    sensors.do_valid = true;
    sensors.do_last_valid = now;
  } else {
    sensors.do_valid = false;
  }

  float co2_reading = read_scaled_value(CO2_PIN, 10.0f);
  sensors.co2 = co2_reading;
  sensors.co2_valid = co2_reading >= 0.0f && co2_reading <= 10.0f;

  float water_level_reading = read_scaled_value(WATER_LEVEL_PIN, 100.0f);
  if (water_level_reading >= 0.0f && water_level_reading <= 100.0f) {
    sensors.water_level = water_level_reading;
    sensors.water_level_valid = true;
    sensors.water_level_last_valid = now;
  } else {
    sensors.water_level_valid = false;
  }

  float aerator_current_reading = read_current_value(AERATOR_CURRENT_PIN);
  if (aerator_current_reading >= 0.0f && aerator_current_reading <= CURRENT_SENSOR_FULL_SCALE_A) {
    sensors.aerator_current = aerator_current_reading;
    sensors.aerator_current_valid = true;
    sensors.aerator_current_last_valid = now;
  } else {
    sensors.aerator_current_valid = false;
  }

  float pump_current_reading = read_current_value(PUMP_CURRENT_PIN);
  if (pump_current_reading >= 0.0f && pump_current_reading <= CURRENT_SENSOR_FULL_SCALE_A) {
    sensors.pump_current = pump_current_reading;
    sensors.pump_current_valid = true;
    sensors.pump_current_last_valid = now;
  } else {
    sensors.pump_current_valid = false;
  }

  sensors.last_read = now;
  update_stale_flags(now);

  Serial.println("===== SENSOR READINGS =====");
  Serial.printf("Water Temp: %.2f C\n", sensors.water_temp);
  Serial.printf("pH: %.2f (rate %.2f pH/h)\n", sensors.ph, sensors.ph_rate);
  Serial.printf("DO: %.2f mg/L\n", sensors.do_value);
  Serial.printf("Water Level: %.2f %%\n", sensors.water_level);
  Serial.printf("Aerator Current: %.2f A | Pump Current: %.2f A\n", sensors.aerator_current, sensors.pump_current);
}

void evaluate_control_logic() {
  const SpeciesRule* rule = getSpeciesRule(current_species);
  uint32_t now = millis();
  update_stale_flags(now);

  OutputState desired = {};
  clearOutputState(desired);

  if (control_state.requested_mode == ControlMode::MANUAL) {
    desired = manual_outputs;
  } else if (control_state.requested_mode == ControlMode::SAFE) {
    desired.aerator1 = true;
    desired.feeder_locked = true;
  } else if (control_state.requested_mode == ControlMode::EMERGENCY) {
    desired.aerator1 = true;
    desired.aerator2 = true;
    desired.alarm = true;
    desired.feeder_locked = true;
  }

  float species_ph_low = maxf(PH_LOW_WARNING, rule->ph_min);
  float species_ph_high = minf(PH_HIGH_WARNING, rule->ph_max);
  float species_temp_warning = minf(TEMP_WARNING, rule->temp_max);
  float species_temp_critical = minf(TEMP_CRITICAL, rule->temp_critical_high);

  SafetyLevel highest_level = SafetyLevel::NORMAL;
  char alarm_message[160] = "Normal";
  char event_key[64] = "normal";
  bool sensor_fault_event = false;
  bool device_fault_event = false;

  bool do_sensor_fault = !sensors.do_valid || sensors.do_stale || sensors.do_last_valid == 0;
  bool ph_warning = (sensors.ph_valid && (sensors.ph < species_ph_low || sensors.ph > species_ph_high)) ||
                    (sensors.ph_valid && sensors.ph_rate > PH_RATE_LIMIT);
  bool temp_warning = sensors.water_temp_valid && sensors.water_temp >= species_temp_warning;
  bool temp_critical = sensors.water_temp_valid && sensors.water_temp >= species_temp_critical;
  bool water_level_low = sensors.water_level_valid && sensors.water_level <= WATER_LEVEL_LOW;
  bool water_level_critical = sensors.water_level_valid && sensors.water_level <= WATER_LEVEL_CRITICAL;
  bool water_level_recovered = sensors.water_level_valid && sensors.water_level >= WATER_LEVEL_RECOVERY;
  bool do_warning = sensors.do_valid && sensors.do_value < DO_WARNING_THRESHOLD;
  bool do_low = sensors.do_valid && sensors.do_value < DO_LOW_THRESHOLD;
  bool do_critical = sensors.do_valid && sensors.do_value < DO_CRITICAL_THRESHOLD;
  bool do_emergency = sensors.do_valid && sensors.do_value < DO_EMERGENCY_THRESHOLD;

  if (control_state.requested_mode == ControlMode::AUTO || control_state.requested_mode == ControlMode::SCHEDULE) {
    if (water_level_low && !water_level_critical) {
      desired.pump = true;
    }
    if (water_level_recovered) {
      desired.pump = false;
    }
    if (temp_warning) {
      desired.circulation = true;
    }
  }

  if (control_state.requested_mode == ControlMode::EMERGENCY) {
    escalate_alarm(highest_level, SafetyLevel::EMERGENCY, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Manual emergency mode active", "alarm");
  }

  if (do_sensor_fault) {
    sensor_fault_event = true;
    escalate_alarm(highest_level, SafetyLevel::SENSOR_FAULT, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "DO sensor invalid or stale - forcing safe aeration", "sensor_fault");
  } else if (do_emergency) {
    escalate_alarm(highest_level, SafetyLevel::EMERGENCY, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "DO below emergency threshold - max aeration required", "alarm");
  } else if (do_critical) {
    escalate_alarm(highest_level, SafetyLevel::CRITICAL, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "DO below critical threshold - safe mode aeration active", "alarm");
  } else if (do_low) {
    escalate_alarm(highest_level, SafetyLevel::LOW, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "DO below low threshold - Aerator 1 forced ON", "alarm");
  } else if (do_warning) {
    escalate_alarm(highest_level, SafetyLevel::WARNING, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "DO below warning threshold", "alarm");
  }

  if (water_level_critical) {
    escalate_alarm(highest_level, SafetyLevel::CRITICAL, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Water level critical - pump protected and feeder locked", "alarm");
  } else if (water_level_low) {
    escalate_alarm(highest_level, SafetyLevel::WARNING, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Water level low - refill pump request active", "alarm");
  }

  if (temp_critical) {
    escalate_alarm(highest_level, SafetyLevel::CRITICAL, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Water temperature critical - aeration forced and feeding locked", "alarm");
  } else if (temp_warning) {
    escalate_alarm(highest_level, SafetyLevel::WARNING, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Water temperature warning", "alarm");
  }

  if (ph_warning) {
    escalate_alarm(highest_level, SafetyLevel::WARNING, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "pH out of preferred range or changing too quickly", "alarm");
  }

  bool pump_current_fault = evaluate_running_current(outputs.pump, sensors.pump_current_valid && !sensors.pump_current_stale,
                                                     sensors.pump_current, control_state.pump_fault_since);
  bool aerator_current_fault = evaluate_running_current(outputs.aerator1 || outputs.aerator2,
                                                        sensors.aerator_current_valid && !sensors.aerator_current_stale,
                                                        sensors.aerator_current, control_state.aerator_fault_since);

  if (pump_current_fault || aerator_current_fault) {
    device_fault_event = true;
    escalate_alarm(highest_level, SafetyLevel::CRITICAL, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Device current out of expected range", "device_fault");
  }

  if (outputs.pump && pump_tracker.on_since > 0 && !water_level_recovered && now - pump_tracker.on_since >= PUMP_MAX_RUN_TIME_MS) {
    device_fault_event = true;
    desired.pump = false;
    escalate_alarm(highest_level, SafetyLevel::CRITICAL, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Pump timeout exceeded without water level recovery", "device_fault");
  }

  if (outputs.feeder && feeder_tracker.on_since > 0 && now - feeder_tracker.on_since >= FEEDER_MAX_RUN_TIME_MS) {
    desired.feeder = false;
    escalate_alarm(highest_level, SafetyLevel::WARNING, alarm_message, sizeof(alarm_message), event_key, sizeof(event_key),
                   "Feeder max runtime reached - feeder stopped", "alarm");
  }

  if (temp_critical) {
    desired.aerator1 = true;
    desired.feeder = false;
    desired.feeder_locked = true;
    desired.alarm = true;
  }

  if (water_level_critical) {
    desired.pump = false;
    desired.feeder = false;
    desired.feeder_locked = true;
    desired.alarm = true;
  }

  apply_safety_outputs(desired, highest_level);

  if (control_state.requested_mode == ControlMode::MANUAL && highest_level != SafetyLevel::NORMAL) {
    desired.pump = desired.pump && !water_level_critical;
    desired.feeder = desired.feeder && !desired.feeder_locked;
  }

  bool recovery_ready = sensors.do_valid && !sensors.do_stale && sensors.do_value >= DO_RECOVERY_THRESHOLD &&
                        (!temp_critical) && (!water_level_critical) && (!sensor_fault_event) && (!device_fault_event);

  if (highest_level == SafetyLevel::NORMAL && control_state.safety_level != SafetyLevel::NORMAL) {
    if (!recovery_ready) {
      control_state.recovery_since = 0;
    } else if (control_state.recovery_since == 0) {
      control_state.recovery_since = now;
    } else if (now - control_state.recovery_since < SAFETY_RECOVERY_HOLD_MS) {
      highest_level = control_state.safety_level;
      copy_text(alarm_message, sizeof(alarm_message), "Recovery hold active before returning to normal");
      copy_text(event_key, sizeof(event_key), "alarm");
      apply_safety_outputs(desired, highest_level);
    }
  } else if (highest_level != SafetyLevel::NORMAL) {
    control_state.recovery_since = 0;
  } else {
    control_state.recovery_since = 0;
  }

  mark_alarm_state(highest_level, alarm_message,
                   sensor_fault_event ? "sensor_fault" : (device_fault_event ? "device_fault" : event_key),
                   highest_level == SafetyLevel::CRITICAL || highest_level == SafetyLevel::EMERGENCY || highest_level == SafetyLevel::SENSOR_FAULT);

  if (highest_level == SafetyLevel::EMERGENCY || control_state.requested_mode == ControlMode::EMERGENCY) {
    control_state.effective_mode = ControlMode::EMERGENCY;
  } else if (highest_level == SafetyLevel::CRITICAL || highest_level == SafetyLevel::SENSOR_FAULT || control_state.requested_mode == ControlMode::SAFE) {
    control_state.effective_mode = ControlMode::SAFE;
  } else {
    control_state.effective_mode = control_state.requested_mode;
  }

  set_output("pump", desired.pump);
  set_output("aerator1", desired.aerator1);
  set_output("aerator2", desired.aerator2);
  set_output("circulation", desired.circulation);
  set_output("feeder", desired.feeder && !desired.feeder_locked);
  set_output("alarm", desired.alarm || manual_outputs.alarm && control_state.requested_mode == ControlMode::MANUAL);
  outputs.feeder_locked = desired.feeder_locked;

  Serial.printf("[RULES] Mode=%s Safety=%s Species=%s Alarm=%s\n",
                controlModeToString(control_state.effective_mode),
                safetyLevelToString(control_state.safety_level),
                rule->name,
                control_state.alarm_text);
}

void set_output(const char* name, bool state, bool force) {
  uint32_t now = millis();
  bool* target_state = nullptr;
  uint8_t pin = 0;
  RelayTracker* tracker = nullptr;
  uint32_t min_interval = OUTPUT_MIN_CHANGE_INTERVAL_MS;

  if (strcmp(name, "pump") == 0) {
    target_state = &outputs.pump;
    pin = PUMP_PIN;
    tracker = &pump_tracker;
    min_interval = state ? PUMP_MIN_OFF_TIME : PUMP_MIN_ON_TIME;
  } else if (strcmp(name, "aerator1") == 0) {
    target_state = &outputs.aerator1;
    pin = AERATOR_1_PIN;
    tracker = &aerator1_tracker;
  } else if (strcmp(name, "aerator2") == 0) {
    target_state = &outputs.aerator2;
    pin = AERATOR_2_PIN;
    tracker = &aerator2_tracker;
  } else if (strcmp(name, "circulation") == 0) {
    target_state = &outputs.circulation;
    pin = CIRCULATION_PIN;
    tracker = &circulation_tracker;
  } else if (strcmp(name, "feeder") == 0) {
    target_state = &outputs.feeder;
    pin = FEEDER_PIN;
    tracker = &feeder_tracker;
  } else if (strcmp(name, "alarm") == 0) {
    target_state = &outputs.alarm;
    pin = ALARM_PIN;
    tracker = &alarm_tracker;
  }

  if (target_state == nullptr || tracker == nullptr) {
    return;
  }

  if (!force && *target_state != state && tracker->last_change > 0 && now - tracker->last_change < min_interval) {
    return;
  }

  if (*target_state == state && !force) {
    return;
  }

  *target_state = state;
  tracker->last_change = now;
  tracker->on_since = state ? now : 0;
  digitalWrite(pin, state ? HIGH : LOW);

  Serial.print("[OUTPUT] ");
  Serial.print(name);
  Serial.println(state ? " ON" : " OFF");
}

void publish_numeric_topic(const char* topic, float value, uint8_t decimals = 2) {
  String payload(value, decimals);
  mqtt_client.publish(topic, payload.c_str(), true);
}

void publish_sensor_data() {
  if (!mqtt_client.connected()) {
    return;
  }

  publish_numeric_topic(MQTT_TOPIC_WATER_TEMP, sensors.water_temp);
  publish_numeric_topic(MQTT_TOPIC_PH, sensors.ph);
  publish_numeric_topic(MQTT_TOPIC_DO, sensors.do_value);
  publish_numeric_topic(MQTT_TOPIC_CO2, sensors.co2);
  publish_numeric_topic(MQTT_TOPIC_AIR_TEMP, sensors.air_temp);
  publish_numeric_topic(MQTT_TOPIC_HUMIDITY, sensors.air_humidity);
  publish_numeric_topic(MQTT_TOPIC_LIGHT, sensors.light, 0);
  publish_numeric_topic(MQTT_TOPIC_WATER_LEVEL, sensors.water_level);
  publish_numeric_topic(MQTT_TOPIC_AERATOR_CURRENT, sensors.aerator_current);
  publish_numeric_topic(MQTT_TOPIC_PUMP_CURRENT, sensors.pump_current);

  String turbidity_payload(static_cast<int>(sensors.turbidity));
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY, turbidity_payload.c_str(), true);
}

void publish_output_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR_1, outputs.aerator1 ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR_2, outputs.aerator2 ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR, (outputs.aerator1 || outputs.aerator2) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_CIRCULATION, outputs.circulation ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_FEEDER, outputs.feeder ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_ALARM_OUTPUT, outputs.alarm ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_MODE_STATE, controlModeToString(control_state.effective_mode), true);
  mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
}

void publish_safety_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_STATUS_SAFETY_STATE, safetyLevelToString(control_state.safety_level), true);
  mqtt_client.publish(MQTT_TOPIC_STATUS_ALARM_TEXT, control_state.alarm_text, true);
  mqtt_client.publish(MQTT_TOPIC_STATUS_ALARM_ACTIVE, control_state.alarm_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_STATUS_SAFETY_ACTIVE, control_state.safety_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_STATUS_EMERGENCY_ACTIVE, control_state.emergency_active ? "ON" : "OFF", true);
}

void publish_heartbeat() {
  if (!mqtt_client.connected()) {
    return;
  }

  StaticJsonDocument<384> doc;
  doc["uptime_s"] = millis() / 1000;
  doc["wifi"] = WiFi.status() == WL_CONNECTED;
  doc["mqtt"] = mqtt_client.connected();
  doc["mode"] = controlModeToString(control_state.effective_mode);
  doc["safety_level"] = safetyLevelToString(control_state.safety_level);

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(MQTT_TOPIC_STATUS_HEARTBEAT, payload.c_str(), false);
}

void publish_controller_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  StaticJsonDocument<768> doc;
  doc["device_id"] = FW_DEVICE_ID;
  doc["firmware"] = FW_VERSION;
  doc["mode"] = controlModeToString(control_state.effective_mode);
  doc["species"] = current_species;
  doc["safety_level"] = safetyLevelToString(control_state.safety_level);
  doc["alarm_text"] = control_state.alarm_text;
  doc["sensors"]["do"] = sensors.do_value;
  doc["sensors"]["ph"] = sensors.ph;
  doc["sensors"]["water_temp"] = sensors.water_temp;
  doc["sensors"]["water_level"] = sensors.water_level;
  doc["outputs"]["pump"] = outputs.pump;
  doc["outputs"]["aerator_1"] = outputs.aerator1;
  doc["outputs"]["aerator_2"] = outputs.aerator2;
  doc["outputs"]["feeder"] = outputs.feeder;
  doc["outputs"]["alarm"] = outputs.alarm;

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(MQTT_TOPIC_STATE, payload.c_str(), true);
}

void publish_event_if_needed(const char* topic) {
  if (!mqtt_client.connected()) {
    return;
  }

  if (strcmp(last_published_event_key, control_state.event_key) == 0) {
    return;
  }

  StaticJsonDocument<512> doc;
  doc["severity"] = safetyLevelToString(control_state.safety_level);
  doc["event"] = control_state.event_key;
  doc["message"] = control_state.alarm_text;
  doc["mode"] = controlModeToString(control_state.effective_mode);
  doc["requires_ack"] = control_state.requires_ack;
  doc["actions"][0] = outputs.feeder_locked ? "inspect_feeder_lock" : "monitor";
  doc["actions"][1] = control_state.safety_level == SafetyLevel::CRITICAL || control_state.safety_level == SafetyLevel::EMERGENCY ? "check_aeration" : "none";

  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(topic, payload.c_str(), false);
  copy_text(last_published_event_key, sizeof(last_published_event_key), control_state.event_key);
}
