#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include "app_config.h"
#include "pins.h"
#include "species_rules.h"
#include "operation_profile.h"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterTemp(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

struct SensorData {
  float water_temp;
  float air_temp;
  float air_humidity;
  float ph;
  float turbidity;
  float do_value;
  float co2;
  float light;
  uint32_t last_read;
};

struct OutputState {
  bool pump;
  bool aerator;
  bool circulation;
  bool feeder;
};

struct ConditionState {
  bool water_temp_valid;
  bool ph_valid;
  bool do_valid;
  bool turbidity_valid;
  bool required_sensors_valid;
  bool sensor_fault_active;
  bool temp_high;
  bool temp_low;
  bool temp_critical_low;
  bool temp_critical_high;
  bool ph_alarm;
  bool do_low;
  bool do_critical;
  bool co2_high;
  bool turbidity_high;
  bool any_active_alarm;
  bool critical_condition_active;
};

SensorData sensors = {};
OutputState outputs = {};
ConditionState conditions = {};

uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;
uint32_t last_pump_change = 0;
uint32_t last_aerator_change = 0;
uint32_t last_circulation_change = 0;
uint32_t last_feeder_change = 0;

char current_mode[16] = "";
char current_species[32] = "Rô Phi";
OperationProfileId requested_profile = PROFILE_SENSOR_TEST;
OperationProfileId actual_profile = PROFILE_SENSOR_TEST;
RelayTestStatusId relay_test_status = RELAY_TEST_NOT_STARTED;
bool livestock_present = false;
bool outputs_locked = true;
bool can_no_load_test = false;
bool can_production = false;
uint16_t requested_blockers = 0;
uint16_t production_blockers = 0;
uint16_t display_blockers = 0;
char block_codes[192] = "none";
char block_summary[256] = "All profile conditions satisfied";
char production_block_summary[256] = "All profile conditions satisfied";
char active_reminder[64] = "ready";

void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void evaluate_conditions();
void update_operation_profile_state(bool force_publish = false);
void apply_species_rules();
void publish_sensor_data();
void publish_output_state();
void publish_profile_state();
void set_output(const char* name, bool state);
bool handle_output_command(const char* output_name, bool requested_state);
void force_outputs_off();
void publish_discovery_sensor(const char* object_id,
                              const char* name,
                              const char* state_topic,
                              const char* icon,
                              const char* unit = nullptr,
                              const char* device_class = nullptr,
                              const char* entity_category = nullptr);
void publish_discovery_binary_sensor(const char* object_id,
                                     const char* name,
                                     const char* state_topic,
                                     const char* icon,
                                     const char* device_class = nullptr,
                                     const char* entity_category = nullptr);
void publish_discovery_switch(const char* object_id,
                              const char* name,
                              const char* state_topic,
                              const char* command_topic,
                              const char* icon);
void publish_discovery_select(const char* object_id,
                              const char* name,
                              const char* state_topic,
                              const char* command_topic,
                              const char* icon,
                              const char* const* options,
                              size_t option_count,
                              const char* entity_category = nullptr);
bool parse_boolean_message(const String& message, bool& value);
void copy_text(char* destination, size_t destination_size, const char* source);

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");

  copy_text(current_mode, sizeof(current_mode), DEFAULT_MODE);
  copy_text(current_species, sizeof(current_species), "Rô Phi");

  pinMode(PUMP_PIN, OUTPUT);
  pinMode(AERATOR_PIN, OUTPUT);
  pinMode(CIRCULATION_PIN, OUTPUT);
  pinMode(FEEDER_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(AERATOR_PIN, LOW);
  digitalWrite(CIRCULATION_PIN, LOW);
  digitalWrite(FEEDER_PIN, LOW);

  Serial.println("[INIT] Initializing sensors...");
  waterTemp.begin();
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL);

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[OK] BH1750 Light Sensor initialized");
  } else {
    Serial.println("[WARN] BH1750 Light Sensor not found (optional)");
  }

  if (SENSOR_TEST_MODE) {
    Serial.println("[INIT] SENSOR_TEST_MODE enabled - outputs are locked OFF");
  }

  setup_wifi();

  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(1024);

  evaluate_conditions();
  update_operation_profile_state();

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
    evaluate_conditions();
    update_operation_profile_state();
    last_sensor_read = now;
  }

  if (now - last_rule_engine >= RULE_ENGINE_INTERVAL && strcmp(current_mode, "AUTO") == 0 && !outputs_locked) {
    apply_species_rules();
    last_rule_engine = now;
  }

  if (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL) {
    if (mqtt_client.connected()) {
      publish_sensor_data();
      publish_output_state();
      publish_profile_state();
      Serial.println("[MQTT] Data published successfully");
    }
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

    mqtt_client.publish(MQTT_TOPIC_STATUS, "online", true);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_PUMP);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_CIRCULATION);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_FEEDER);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_MODE);
    mqtt_client.subscribe(MQTT_TOPIC_CONFIG_SPECIES);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_OPERATION_PROFILE);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_LIVESTOCK_PRESENT);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_RELAY_TEST);

    publish_mqtt_discovery();
    publish_sensor_data();
    publish_output_state();
    publish_profile_state();
  } else {
    Serial.print("[WARN] MQTT connection failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

void publish_mqtt_discovery() {
  static const char* const mode_options[] = {"AUTO", "MANUAL", "SCHEDULE", "SAFE"};
  static const char* const species_options[] = {"Koi", "Cá Trắm", "Cá Chép", "Cá Tra", "Cá Lóc", "Tôm Thẻ", "Tôm Sú", "Tilapia"};
  static const char* const profile_options[] = {"SENSOR_TEST", "NO_LIVESTOCK_TEST", "PRODUCTION"};
  static const char* const relay_test_options[] = {"NOT_STARTED", "IN_PROGRESS", "PASSED", "FAILED"};

  Serial.println("[HA Discovery] Publishing entity discoveries...");

  publish_discovery_sensor("aquaculture_water_temp", "Aquaculture Water Temperature", MQTT_TOPIC_WATER_TEMP, "mdi:thermometer", "°C", "temperature");
  publish_discovery_sensor("aquaculture_ph", "Aquaculture pH", MQTT_TOPIC_PH, "mdi:test-tube", "pH");
  publish_discovery_sensor("aquaculture_do", "Aquaculture Dissolved Oxygen", MQTT_TOPIC_DO, "mdi:water", "mg/L");
  publish_discovery_sensor("aquaculture_co2", "Aquaculture CO2", MQTT_TOPIC_CO2, "mdi:molecule-co2", "ppm");
  publish_discovery_sensor("aquaculture_turbidity", "Aquaculture Turbidity", MQTT_TOPIC_TURBIDITY, "mdi:water-opacity", "NTU");
  publish_discovery_sensor("aquaculture_air_temp", "Aquaculture Air Temperature", MQTT_TOPIC_AIR_TEMP, "mdi:thermometer", "°C", "temperature");
  publish_discovery_sensor("aquaculture_humidity", "Aquaculture Humidity", MQTT_TOPIC_HUMIDITY, "mdi:water-percent", "%", "humidity");
  publish_discovery_sensor("aquaculture_light", "Aquaculture Light Level", MQTT_TOPIC_LIGHT, "mdi:lightbulb", "lux", "illuminance");

  publish_discovery_switch("aquaculture_pump", "Aquaculture Pump", MQTT_TOPIC_PUMP, MQTT_TOPIC_CONTROL_PUMP, "mdi:pump");
  publish_discovery_switch("aquaculture_aerator", "Aquaculture Aerator", MQTT_TOPIC_AERATOR, MQTT_TOPIC_CONTROL_AERATOR, "mdi:air-purifier");
  publish_discovery_switch("aquaculture_circulation", "Aquaculture Circulation", MQTT_TOPIC_CIRCULATION, MQTT_TOPIC_CONTROL_CIRCULATION, "mdi:water-pump");
  publish_discovery_switch("aquaculture_feeder", "Aquaculture Feeder", MQTT_TOPIC_FEEDER, MQTT_TOPIC_CONTROL_FEEDER, "mdi:fish-food");
  publish_discovery_switch("aquaculture_livestock_present", "Aquaculture Livestock Present", MQTT_TOPIC_LIVESTOCK_PRESENT_STATE, MQTT_TOPIC_CONTROL_LIVESTOCK_PRESENT, "mdi:fish");

  publish_discovery_select("aquaculture_mode", "Aquaculture Mode", MQTT_TOPIC_MODE_STATE, MQTT_TOPIC_CONTROL_MODE, "mdi:cog", mode_options, 4, "config");
  publish_discovery_select("aquaculture_species", "Aquaculture Species", MQTT_TOPIC_SPECIES_STATE, MQTT_TOPIC_CONFIG_SPECIES, "mdi:fishbowl", species_options, 8, "config");
  publish_discovery_select("aquaculture_operation_profile", "Aquaculture Operation Profile", MQTT_TOPIC_OPERATION_PROFILE_SELECTED, MQTT_TOPIC_CONTROL_OPERATION_PROFILE, "mdi:shield-check", profile_options, 3, "config");
  publish_discovery_select("aquaculture_relay_test_status", "Aquaculture Relay Test Status", MQTT_TOPIC_RELAY_TEST_STATE, MQTT_TOPIC_CONTROL_RELAY_TEST, "mdi:toggle-switch", relay_test_options, 4, "config");

  publish_discovery_sensor("aquaculture_active_profile", "Aquaculture Active Profile", MQTT_TOPIC_OPERATION_PROFILE_ACTUAL, "mdi:shield-account");
  publish_discovery_binary_sensor("aquaculture_can_no_load_test", "Aquaculture Can No-Load Test", MQTT_TOPIC_CAN_NO_LOAD_TEST, "mdi:beaker-check");
  publish_discovery_binary_sensor("aquaculture_can_production", "Aquaculture Can Production", MQTT_TOPIC_CAN_PRODUCTION, "mdi:fish");
  publish_discovery_binary_sensor("aquaculture_outputs_locked", "Aquaculture Outputs Locked", MQTT_TOPIC_OUTPUTS_LOCKED, "mdi:lock");
  publish_discovery_binary_sensor("aquaculture_required_sensors_valid", "Aquaculture Required Sensors Valid", MQTT_TOPIC_REQUIRED_SENSORS_VALID, "mdi:check-decagram");
  publish_discovery_binary_sensor("aquaculture_sensor_fault", "Aquaculture Sensor Fault", MQTT_TOPIC_SENSOR_FAULT_ACTIVE, "mdi:alert-circle", "problem");
  publish_discovery_binary_sensor("aquaculture_any_active_alarm", "Aquaculture Any Active Alarm", MQTT_TOPIC_ANY_ACTIVE_ALARM, "mdi:alarm-light", "problem");
  publish_discovery_binary_sensor("aquaculture_critical_condition", "Aquaculture Critical Condition", MQTT_TOPIC_CRITICAL_CONDITION_ACTIVE, "mdi:alert-octagon", "problem");
  publish_discovery_sensor("aquaculture_profile_block_codes", "Aquaculture Profile Block Codes", MQTT_TOPIC_PROFILE_BLOCK_CODES, "mdi:code-tags");
  publish_discovery_sensor("aquaculture_profile_block_summary", "Aquaculture Operation Profile Block Summary", MQTT_TOPIC_PROFILE_BLOCK_SUMMARY, "mdi:information-outline");
  publish_discovery_sensor("aquaculture_production_block_reason", "Aquaculture Production Block Reason", MQTT_TOPIC_PRODUCTION_BLOCK_SUMMARY, "mdi:message-alert");
  publish_discovery_sensor("aquaculture_active_reminder", "Aquaculture Active Reminder", MQTT_TOPIC_ACTIVE_REMINDER, "mdi:bell-alert");

  publish_discovery_binary_sensor("aquaculture_block_required_sensor_invalid", "Aquaculture Block Required Sensor Invalid", MQTT_TOPIC_BLOCK_REQUIRED_SENSOR_INVALID, "mdi:thermometer-alert", "problem");
  publish_discovery_binary_sensor("aquaculture_block_sensor_fault_active", "Aquaculture Block Sensor Fault Active", MQTT_TOPIC_BLOCK_SENSOR_FAULT_ACTIVE, "mdi:alert-circle", "problem");
  publish_discovery_binary_sensor("aquaculture_block_relay_test_not_passed", "Aquaculture Block Relay Test Not Passed", MQTT_TOPIC_BLOCK_RELAY_TEST_NOT_PASSED, "mdi:toggle-switch-off", "problem");
  publish_discovery_binary_sensor("aquaculture_block_outputs_locked", "Aquaculture Block Outputs Locked", MQTT_TOPIC_BLOCK_OUTPUTS_LOCKED, "mdi:lock-outline", "problem");
  publish_discovery_binary_sensor("aquaculture_block_sensor_test_mode_enabled", "Aquaculture Block Sensor Test Mode Enabled", MQTT_TOPIC_BLOCK_SENSOR_TEST_MODE_ENABLED, "mdi:test-tube", "problem");
  publish_discovery_binary_sensor("aquaculture_block_active_alarm", "Aquaculture Block Active Alarm", MQTT_TOPIC_BLOCK_ACTIVE_ALARM, "mdi:alarm-light-outline", "problem");
  publish_discovery_binary_sensor("aquaculture_block_livestock_not_confirmed", "Aquaculture Block Livestock Not Confirmed", MQTT_TOPIC_BLOCK_LIVESTOCK_NOT_CONFIRMED, "mdi:fish-off", "problem");
  publish_discovery_binary_sensor("aquaculture_block_critical_condition_active", "Aquaculture Block Critical Condition Active", MQTT_TOPIC_BLOCK_CRITICAL_CONDITION_ACTIVE, "mdi:alert-octagon-outline", "problem");

  Serial.println("[OK] All discoveries published!");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }
  message.trim();

  Serial.print("[MQTT] Message received on: ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(message);

  if (strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0) {
    copy_text(current_mode, sizeof(current_mode), "MANUAL");
    handle_output_command("pump", message == "ON");
    publish_output_state();
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR) == 0) {
    copy_text(current_mode, sizeof(current_mode), "MANUAL");
    handle_output_command("aerator", message == "ON");
    publish_output_state();
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_CIRCULATION) == 0) {
    copy_text(current_mode, sizeof(current_mode), "MANUAL");
    handle_output_command("circulation", message == "ON");
    publish_output_state();
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_FEEDER) == 0) {
    copy_text(current_mode, sizeof(current_mode), "MANUAL");
    handle_output_command("feeder", message == "ON");
    publish_output_state();
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_MODE) == 0) {
    copy_text(current_mode, sizeof(current_mode), message.c_str());
    Serial.print("[CONFIG] Mode changed to: ");
    Serial.println(current_mode);
    mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONFIG_SPECIES) == 0) {
    copy_text(current_species, sizeof(current_species), message.c_str());
    Serial.print("[CONFIG] Species changed to: ");
    Serial.println(current_species);
    evaluate_conditions();
    update_operation_profile_state(true);
    mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_OPERATION_PROFILE) == 0) {
    requested_profile = operationProfileFromString(message.c_str());
    Serial.print("[PROFILE] Requested profile -> ");
    Serial.println(operationProfileToString(requested_profile));
    update_operation_profile_state(true);
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_LIVESTOCK_PRESENT) == 0) {
    bool parsed_value = false;
    if (parse_boolean_message(message, parsed_value)) {
      livestock_present = parsed_value;
      Serial.print("[PROFILE] livestock_present -> ");
      Serial.println(livestock_present ? "ON" : "OFF");
      update_operation_profile_state(true);
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_RELAY_TEST) == 0) {
    relay_test_status = relayTestStatusFromString(message.c_str());
    Serial.print("[PROFILE] relay_test_status -> ");
    Serial.println(relayTestStatusToString(relay_test_status));
    update_operation_profile_state(true);
    return;
  }
}

void read_sensors() {
  waterTemp.requestTemperatures();
  sensors.water_temp = waterTemp.getTempCByIndex(0);
  if (sensors.water_temp == -127) {
    sensors.water_temp = 0;
  }

  sensors.air_temp = dht.readTemperature();
  sensors.air_humidity = dht.readHumidity();
  if (isnan(sensors.air_temp)) {
    sensors.air_temp = 0;
  }
  if (isnan(sensors.air_humidity)) {
    sensors.air_humidity = 0;
  }

  sensors.light = lightMeter.readLightLevel();
  if (sensors.light < 0) {
    sensors.light = 0;
  }

  sensors.ph = (analogRead(PH_PIN) / 4095.0f) * 14.0f;
  sensors.turbidity = analogRead(TURBIDITY_PIN);
  sensors.do_value = (analogRead(DO_PIN) / 4095.0f) * 20.0f;
  sensors.co2 = (analogRead(CO2_PIN) / 4095.0f) * 10.0f;
  sensors.last_read = millis();

  Serial.println("===== SENSOR READINGS =====");
  Serial.print("Water Temp: ");
  Serial.print(sensors.water_temp);
  Serial.println("°C");
  Serial.print("pH: ");
  Serial.println(sensors.ph);
  Serial.print("DO: ");
  Serial.print(sensors.do_value);
  Serial.println(" mg/L");
  Serial.print("CO2: ");
  Serial.print(sensors.co2);
  Serial.println(" ppm");
  Serial.print("Turbidity: ");
  Serial.println(sensors.turbidity);
  Serial.print("Air Temp: ");
  Serial.print(sensors.air_temp);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.println(sensors.air_humidity);
  Serial.print("%");
  Serial.print("Light: ");
  Serial.print(sensors.light);
  Serial.println(" lux");
}

void evaluate_conditions() {
  const SpeciesRule* rule = getSpeciesRule(current_species);

  conditions.water_temp_valid = sensors.water_temp > 0.0f && sensors.water_temp <= 60.0f;
  conditions.ph_valid = sensors.ph > 0.0f && sensors.ph <= 14.0f;
  conditions.do_valid = sensors.do_value > 0.0f && sensors.do_value <= 20.0f;
  conditions.turbidity_valid = sensors.turbidity >= 0.0f && sensors.turbidity <= 4095.0f;
  conditions.required_sensors_valid = conditions.water_temp_valid && conditions.ph_valid && conditions.do_valid && conditions.turbidity_valid;
  conditions.sensor_fault_active = !conditions.required_sensors_valid;

  conditions.temp_high = conditions.water_temp_valid && sensors.water_temp > rule->temp_max;
  conditions.temp_low = conditions.water_temp_valid && sensors.water_temp < rule->temp_min;
  conditions.temp_critical_low = conditions.water_temp_valid && sensors.water_temp <= rule->temp_critical_low;
  conditions.temp_critical_high = conditions.water_temp_valid && sensors.water_temp >= rule->temp_critical_high;
  conditions.ph_alarm = conditions.ph_valid && (sensors.ph < rule->ph_min || sensors.ph > rule->ph_max);
  conditions.do_low = conditions.do_valid && sensors.do_value < rule->do_min;
  conditions.do_critical = conditions.do_valid && sensors.do_value <= rule->do_critical;
  conditions.co2_high = sensors.co2 > rule->co2_max;
  conditions.turbidity_high = conditions.turbidity_valid && sensors.turbidity > rule->turbidity_max;

  conditions.any_active_alarm =
      conditions.temp_high ||
      conditions.temp_low ||
      conditions.ph_alarm ||
      conditions.do_low ||
      conditions.do_critical ||
      conditions.co2_high ||
      conditions.turbidity_high;

  conditions.critical_condition_active =
      conditions.do_critical ||
      conditions.temp_critical_low ||
      conditions.temp_critical_high;
}

void update_operation_profile_state(bool force_publish) {
  ProfileEligibilityInputs inputs = {
      conditions.required_sensors_valid,
      conditions.sensor_fault_active,
      relay_test_status == RELAY_TEST_PASSED,
      SENSOR_TEST_MODE,
      SENSOR_TEST_MODE,
      conditions.any_active_alarm,
      conditions.critical_condition_active,
      livestock_present};

  ProfileEvaluation evaluation = evaluateProfileRequest(requested_profile, inputs);
  actual_profile = evaluation.actual_profile;
  can_no_load_test = evaluation.can_no_load_test;
  can_production = evaluation.can_production;
  requested_blockers = evaluation.requested_blockers;
  production_blockers = evaluation.production_blockers;
  display_blockers = requested_profile == PROFILE_SENSOR_TEST ? production_blockers : requested_blockers;
  outputs_locked = SENSOR_TEST_MODE || actual_profile == PROFILE_SENSOR_TEST;

  buildReasonList(display_blockers, false, block_codes, sizeof(block_codes));
  buildReasonList(display_blockers, true, block_summary, sizeof(block_summary));
  buildReasonList(production_blockers, true, production_block_summary, sizeof(production_block_summary));
  copy_text(active_reminder, sizeof(active_reminder), firstActiveReminder(display_blockers));

  if (outputs_locked) {
    force_outputs_off();
  }

  static bool first_log = true;
  static OperationProfileId last_logged_actual = PROFILE_SENSOR_TEST;
  static bool last_logged_can_no_load = false;
  static bool last_logged_can_production = false;
  static uint16_t last_logged_display_blockers = 0xFFFF;

  if (first_log || force_publish || actual_profile != last_logged_actual) {
    if (actual_profile == requested_profile) {
      Serial.print("[PROFILE] Actual profile -> ");
      Serial.println(operationProfileToString(actual_profile));
    } else {
      Serial.print("[PROFILE] Requested ");
      Serial.print(operationProfileToString(requested_profile));
      Serial.print(" downgraded to ");
      Serial.print(operationProfileToString(actual_profile));
      Serial.print(" (");
      Serial.print(block_codes);
      Serial.println(")");
    }
  }

  if (first_log || force_publish || can_no_load_test != last_logged_can_no_load) {
    Serial.print("[PROFILE] can_no_load_test -> ");
    Serial.println(can_no_load_test ? "ON" : "OFF");
  }

  if (first_log || force_publish || can_production != last_logged_can_production) {
    Serial.print("[PROFILE] can_production -> ");
    Serial.println(can_production ? "ON" : "OFF");
  }

  if (first_log || force_publish || display_blockers != last_logged_display_blockers) {
    Serial.print("[REMINDER] active=");
    Serial.print(active_reminder);
    Serial.print(" summary=");
    Serial.println(block_summary);
  }

  last_logged_actual = actual_profile;
  last_logged_can_no_load = can_no_load_test;
  last_logged_can_production = can_production;
  last_logged_display_blockers = display_blockers;
  first_log = false;

  if (mqtt_client.connected()) {
    publish_profile_state();
  }
}

void apply_species_rules() {
  const SpeciesRule* rule = getSpeciesRule(current_species);

  bool pump_on = outputs.pump;
  bool aerator_on = outputs.aerator;
  bool circulation_on = outputs.circulation;
  bool feeder_on = outputs.feeder;

  if (conditions.do_critical) {
    aerator_on = true;
    pump_on = true;
  }

  if (conditions.temp_critical_high) {
    circulation_on = true;
    pump_on = true;
  }

  if (conditions.do_low) {
    aerator_on = true;
  }

  if (conditions.temp_high) {
    circulation_on = true;
  }

  if (conditions.co2_high) {
    pump_on = true;
    aerator_on = true;
  }

  if (conditions.do_valid && sensors.do_value < rule->do_min + 0.5f) {
    circulation_on = true;
  }

  feeder_on = false;

  uint32_t now = millis();

  if (pump_on != outputs.pump && now - last_pump_change > PUMP_MIN_OFF_TIME) {
    set_output("pump", pump_on);
    last_pump_change = now;
  }

  if (aerator_on != outputs.aerator && now - last_aerator_change > 5000) {
    set_output("aerator", aerator_on);
    last_aerator_change = now;
  }

  if (circulation_on != outputs.circulation && now - last_circulation_change > 5000) {
    set_output("circulation", circulation_on);
    last_circulation_change = now;
  }

  if (feeder_on != outputs.feeder && now - last_feeder_change > 60000) {
    set_output("feeder", feeder_on);
    last_feeder_change = now;
  }

  if (conditions.any_active_alarm) {
    Serial.println("[RULES] Species-based control activated with alerts");
  }

  Serial.print("[RULES] Active species: ");
  Serial.println(rule->name);
}

void set_output(const char* name, bool state) {
  if (strcmp(name, "pump") == 0) {
    outputs.pump = state;
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
  } else if (strcmp(name, "aerator") == 0) {
    outputs.aerator = state;
    digitalWrite(AERATOR_PIN, state ? HIGH : LOW);
  } else if (strcmp(name, "circulation") == 0) {
    outputs.circulation = state;
    digitalWrite(CIRCULATION_PIN, state ? HIGH : LOW);
  } else if (strcmp(name, "feeder") == 0) {
    outputs.feeder = state;
    digitalWrite(FEEDER_PIN, state ? HIGH : LOW);
  }

  Serial.print("[OUTPUT] ");
  Serial.print(name);
  Serial.println(state ? " ON" : " OFF");
}

bool handle_output_command(const char* output_name, bool requested_state) {
  if (requested_state && outputs_locked) {
    Serial.print("[TEST] Suppressed ON command for ");
    Serial.println(output_name);
    set_output(output_name, false);
    return false;
  }

  set_output(output_name, requested_state);
  return true;
}

void force_outputs_off() {
  if (outputs.pump) {
    set_output("pump", false);
  }
  if (outputs.aerator) {
    set_output("aerator", false);
  }
  if (outputs.circulation) {
    set_output("circulation", false);
  }
  if (outputs.feeder) {
    set_output("feeder", false);
  }
}

void publish_sensor_data() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP, String(sensors.water_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH, String(sensors.ph, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO, String(sensors.do_value, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_CO2, String(sensors.co2, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY, String(sensors.turbidity, 0).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_AIR_TEMP, String(sensors.air_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY, String(sensors.air_humidity, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_LIGHT, String(sensors.light, 0).c_str(), true);
}

void publish_output_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR, outputs.aerator ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_CIRCULATION, outputs.circulation ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_FEEDER, outputs.feeder ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
  mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
}

void publish_profile_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_OPERATION_PROFILE_SELECTED, operationProfileToString(requested_profile), true);
  mqtt_client.publish(MQTT_TOPIC_OPERATION_PROFILE_ACTUAL, operationProfileToString(actual_profile), true);
  mqtt_client.publish(MQTT_TOPIC_LIVESTOCK_PRESENT_STATE, livestock_present ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_RELAY_TEST_STATE, relayTestStatusToString(relay_test_status), true);
  mqtt_client.publish(MQTT_TOPIC_CAN_NO_LOAD_TEST, can_no_load_test ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_CAN_PRODUCTION, can_production ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_OUTPUTS_LOCKED, outputs_locked ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_REQUIRED_SENSORS_VALID, conditions.required_sensors_valid ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_SENSOR_FAULT_ACTIVE, conditions.sensor_fault_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_ANY_ACTIVE_ALARM, conditions.any_active_alarm ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_CRITICAL_CONDITION_ACTIVE, conditions.critical_condition_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_PROFILE_BLOCK_CODES, block_codes, true);
  mqtt_client.publish(MQTT_TOPIC_PROFILE_BLOCK_SUMMARY, block_summary, true);
  mqtt_client.publish(MQTT_TOPIC_PRODUCTION_BLOCK_SUMMARY, production_block_summary, true);
  mqtt_client.publish(MQTT_TOPIC_ACTIVE_REMINDER, active_reminder, true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_REQUIRED_SENSOR_INVALID, (display_blockers & BLOCK_REQUIRED_SENSOR_INVALID) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_SENSOR_FAULT_ACTIVE, (display_blockers & BLOCK_SENSOR_FAULT_ACTIVE) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_RELAY_TEST_NOT_PASSED, (display_blockers & BLOCK_RELAY_TEST_NOT_PASSED) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_OUTPUTS_LOCKED, (display_blockers & BLOCK_OUTPUTS_LOCKED) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_SENSOR_TEST_MODE_ENABLED, (display_blockers & BLOCK_SENSOR_TEST_MODE_ENABLED) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_ACTIVE_ALARM, (display_blockers & BLOCK_ACTIVE_ALARM) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_LIVESTOCK_NOT_CONFIRMED, (display_blockers & BLOCK_LIVESTOCK_NOT_CONFIRMED) ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_BLOCK_CRITICAL_CONDITION_ACTIVE, (display_blockers & BLOCK_CRITICAL_CONDITION_ACTIVE) ? "ON" : "OFF", true);
}

template <typename T>
void configure_discovery_device(T& doc) {
  doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
  doc["device"]["name"] = "Aquaculture Controller";
  doc["device"]["model"] = "ESP32 Aquaculture V0.3";
  doc["device"]["manufacturer"] = "SmartFarm";
}

void publish_discovery_sensor(const char* object_id,
                              const char* name,
                              const char* state_topic,
                              const char* icon,
                              const char* unit,
                              const char* device_class,
                              const char* entity_category) {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["icon"] = icon;
  if (unit != nullptr) {
    doc["unit_of_measurement"] = unit;
  }
  if (device_class != nullptr) {
    doc["device_class"] = device_class;
  }
  if (entity_category != nullptr) {
    doc["entity_category"] = entity_category;
  }
  configure_discovery_device(doc);

  String payload;
  serializeJson(doc, payload);
  String topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + object_id + "/config";
  mqtt_client.publish(topic.c_str(), payload.c_str(), true);
}

void publish_discovery_binary_sensor(const char* object_id,
                                     const char* name,
                                     const char* state_topic,
                                     const char* icon,
                                     const char* device_class,
                                     const char* entity_category) {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["icon"] = icon;
  if (device_class != nullptr) {
    doc["device_class"] = device_class;
  }
  if (entity_category != nullptr) {
    doc["entity_category"] = entity_category;
  }
  configure_discovery_device(doc);

  String payload;
  serializeJson(doc, payload);
  String topic = String(HA_DISCOVERY_PREFIX) + "/binary_sensor/" + object_id + "/config";
  mqtt_client.publish(topic.c_str(), payload.c_str(), true);
}

void publish_discovery_switch(const char* object_id,
                              const char* name,
                              const char* state_topic,
                              const char* command_topic,
                              const char* icon) {
  StaticJsonDocument<512> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["icon"] = icon;
  configure_discovery_device(doc);

  String payload;
  serializeJson(doc, payload);
  String topic = String(HA_DISCOVERY_PREFIX) + "/switch/" + object_id + "/config";
  mqtt_client.publish(topic.c_str(), payload.c_str(), true);
}

void publish_discovery_select(const char* object_id,
                              const char* name,
                              const char* state_topic,
                              const char* command_topic,
                              const char* icon,
                              const char* const* options,
                              size_t option_count,
                              const char* entity_category) {
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = object_id;
  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["icon"] = icon;
  if (entity_category != nullptr) {
    doc["entity_category"] = entity_category;
  }
  JsonArray json_options = doc.createNestedArray("options");
  for (size_t i = 0; i < option_count; ++i) {
    json_options.add(options[i]);
  }
  configure_discovery_device(doc);

  String payload;
  serializeJson(doc, payload);
  String topic = String(HA_DISCOVERY_PREFIX) + "/select/" + object_id + "/config";
  mqtt_client.publish(topic.c_str(), payload.c_str(), true);
}

bool parse_boolean_message(const String& message, bool& value) {
  String normalized = message;
  normalized.trim();
  normalized.toUpperCase();

  if (normalized == "ON" || normalized == "TRUE" || normalized == "1" || normalized == "YES") {
    value = true;
    return true;
  }

  if (normalized == "OFF" || normalized == "FALSE" || normalized == "0" || normalized == "NO") {
    value = false;
    return true;
  }

  Serial.print("[WARN] Unsupported boolean payload: ");
  Serial.println(message);
  return false;
}

void copy_text(char* destination, size_t destination_size, const char* source) {
  if (destination_size == 0) {
    return;
  }

  strncpy(destination, source, destination_size - 1);
  destination[destination_size - 1] = '\0';
}
