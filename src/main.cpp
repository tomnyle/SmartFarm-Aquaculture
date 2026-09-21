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

// ==================== GLOBAL OBJECTS ====================
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterTemp(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

// ==================== SENSOR DATA STRUCTURE ====================
struct SensorData {
  float water_temp;
  float air_temp;
  float air_humidity;
  float ph;
  float ph_trend;
  float turbidity;
  float do_value;
  float co2;
  float light;
  float water_level;
  float aerator_current;
  float pump_current;
  float ph_voltage;
  float do_voltage;
  float turbidity_voltage;
  int ph_raw;
  int do_raw;
  int turbidity_raw;
  int water_level_raw;
  int aerator_current_raw;
  int pump_current_raw;
  uint32_t last_read;
  float last_ph;
  uint32_t last_ph_timestamp;
  bool has_last_ph;
};

struct OutputState {
  bool pump;
  bool aerator;
  bool aerator_2;
  bool circulation;
  bool feeder;
  bool alarm_output;
};

// ==================== GLOBAL VARIABLES ====================
SensorData sensors;
OutputState outputs;

uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;
uint32_t last_pump_change = 0;
uint32_t last_aerator_change = 0;
uint32_t last_circulation_change = 0;
uint32_t last_feeder_change = 0;

char current_mode[16] = "";
char current_species[32] = "Rô Phi";
char mqtt_runtime_client_id[96] = "";
char device_identifier[64] = "";
char device_display_name[64] = "";
char unique_id_prefix[96] = "";
char controller_status[24] = "INITIALIZING";
char safety_state[24] = "SAFE";
char alarm_text[128] = "No alarms";

bool light_sensor_available = false;
bool alarm_active = false;
bool safety_active = false;
bool emergency_active = false;
bool mqtt_connection_attempted = false;
bool mqtt_connected_once = false;
bool required_analog_pins_valid = true;

// ==================== FORWARD DECLARATIONS ====================
void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void apply_species_rules();
void publish_sensor_data();
void publish_output_state();
void set_output(const char* name, bool state);
void initialize_device_identity();
void update_system_status_flags();
bool is_supported_mode(const char* mode);
bool is_supported_species(const char* species);
void publish_runtime_status_topics();
bool has_water_level_runtime_data();
bool has_aerator_current_runtime_data();
bool has_pump_current_runtime_data();
bool has_co2_runtime_data();
bool has_required_analog_runtime_data();
bool outputs_locked_for_test();
bool is_adc1_pin(int pin);
int read_analog_average(int pin, int samples);
float clamp_float(float value, float min_value, float max_value);
float adc_to_voltage(int adc_raw);
int voltage_to_adc(float voltage);
float read_ph_from_adc(int adc_raw);
float read_do_from_adc(int adc_raw);
float read_co2_from_adc(int adc_raw);
float read_turbidity_from_adc(int adc_raw);
float read_water_level_from_adc(int adc_raw);
float read_current_from_adc(int adc_raw, float zero_voltage, float amp_per_volt, float max_value);
bool publish_float_topic(const char* topic, float value);

void initialize_device_identity() {
  uint64_t chip_id = ESP.getEfuseMac();
  char mac_suffix[13];
  snprintf(
    mac_suffix,
    sizeof(mac_suffix),
    "%02X%02X%02X%02X%02X%02X",
    (uint8_t)(chip_id >> 40),
    (uint8_t)(chip_id >> 32),
    (uint8_t)(chip_id >> 24),
    (uint8_t)(chip_id >> 16),
    (uint8_t)(chip_id >> 8),
    (uint8_t)(chip_id)
  );

  snprintf(mqtt_runtime_client_id, sizeof(mqtt_runtime_client_id), "%s_%s", MQTT_CLIENT_ID, mac_suffix);
  snprintf(device_identifier, sizeof(device_identifier), "%s_%s", FW_DEVICE_ID, mac_suffix);
  snprintf(unique_id_prefix, sizeof(unique_id_prefix), "aquaculture_v2_%s", mac_suffix);
  snprintf(device_display_name, sizeof(device_display_name), "%s", DEVICE_DISPLAY_NAME);
}

String build_discovery_topic(const char* component, const char* object_id) {
  String topic = HA_DISCOVERY_PREFIX;
  topic += "/";
  topic += component;
  topic += "/";
  topic += object_id;
  topic += "/config";
  return topic;
}

String build_unique_id(const char* key) {
  String uid = unique_id_prefix;
  uid += "_";
  uid += key;
  return uid;
}

void attach_device_metadata(JsonDocument& doc) {
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(device_identifier);

  device["name"] = device_display_name;
  device["manufacturer"] = DEVICE_MANUFACTURER;
  device["model"] = DEVICE_MODEL;
  device["sw_version"] = FW_VERSION;
  device["hw_version"] = FW_HW_VERSION;
  device["configuration_url"] = DEVICE_CONFIG_URL;
}

void set_common_availability(JsonDocument& doc, const char* availability_topic) {
  doc["availability_topic"] = availability_topic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
}

void publish_discovery_payload(const char* component, const char* object_id, JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  mqtt_client.publish(build_discovery_topic(component, object_id).c_str(), payload.c_str(), true);
}

void publish_sensor_discovery(
  const char* object_id,
  const char* name,
  const char* state_topic,
  const char* unit,
  const char* icon,
  const char* device_class,
  const char* state_class,
  const char* availability_topic,
  bool enabled_by_default = true
) {
  StaticJsonDocument<1024> doc;
  doc["name"] = name;
  doc["unique_id"] = build_unique_id(object_id);
  doc["state_topic"] = state_topic;

  if (unit != nullptr && strlen(unit) > 0) doc["unit_of_measurement"] = unit;
  if (icon != nullptr && strlen(icon) > 0) doc["icon"] = icon;
  if (device_class != nullptr && strlen(device_class) > 0) doc["device_class"] = device_class;
  if (state_class != nullptr && strlen(state_class) > 0) doc["state_class"] = state_class;

  doc["enabled_by_default"] = enabled_by_default;

  set_common_availability(doc, availability_topic);
  attach_device_metadata(doc);
  publish_discovery_payload("sensor", object_id, doc);
}

void publish_binary_sensor_discovery(
  const char* object_id,
  const char* name,
  const char* state_topic,
  const char* icon,
  const char* device_class,
  const char* availability_topic
) {
  StaticJsonDocument<768> doc;
  doc["name"] = name;
  doc["unique_id"] = build_unique_id(object_id);
  doc["state_topic"] = state_topic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";

  if (icon != nullptr && strlen(icon) > 0) doc["icon"] = icon;
  if (device_class != nullptr && strlen(device_class) > 0) doc["device_class"] = device_class;

  set_common_availability(doc, availability_topic);
  attach_device_metadata(doc);
  publish_discovery_payload("binary_sensor", object_id, doc);
}

void publish_switch_discovery(
  const char* object_id,
  const char* name,
  const char* state_topic,
  const char* command_topic,
  const char* icon,
  const char* availability_topic,
  bool enabled_by_default = true
) {
  StaticJsonDocument<1024> doc;
  doc["name"] = name;
  doc["unique_id"] = build_unique_id(object_id);
  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["enabled_by_default"] = enabled_by_default;

  if (icon != nullptr && strlen(icon) > 0) doc["icon"] = icon;

  set_common_availability(doc, availability_topic);
  attach_device_metadata(doc);
  publish_discovery_payload("switch", object_id, doc);
}

bool is_supported_mode(const char* mode) {
  return
    strcmp(mode, "AUTO") == 0 ||
    strcmp(mode, "MANUAL") == 0 ||
    strcmp(mode, "SCHEDULE") == 0 ||
    strcmp(mode, "SAFE") == 0;
}

bool is_supported_species(const char* species) {
  return
    strcmp(species, "Koi") == 0 ||
    strcmp(species, "Cá Trắm") == 0 ||
    strcmp(species, "Cá Chép") == 0 ||
    strcmp(species, "Cá Tra") == 0 ||
    strcmp(species, "Cá Lóc") == 0 ||
    strcmp(species, "Tôm Thẻ") == 0 ||
    strcmp(species, "Tôm Sú") == 0 ||
    strcmp(species, "Rô Phi") == 0 ||
    strcmp(species, "Tilapia") == 0;
}

bool outputs_locked_for_test() {
  return BENCH_TEST_MODE || SENSOR_TEST_MODE;
}

bool is_adc1_pin(int pin) {
  switch (pin) {
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
      return true;
    default:
      return false;
  }
}

int read_analog_average(int pin, int samples) {
  if (!is_adc1_pin(pin) || samples <= 0) {
    return -1;
  }

  uint32_t total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    if (ANALOG_SAMPLE_DELAY_US > 0) {
      delayMicroseconds(ANALOG_SAMPLE_DELAY_US);
    }
  }
  return (int)(total / (uint32_t)samples);
}

float clamp_float(float value, float min_value, float max_value) {
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

float adc_to_voltage(int adc_raw) {
  if (adc_raw < 0) {
    return NAN;
  }
  return ((float)adc_raw / 4095.0f) * ADC_REFERENCE_VOLTAGE;
}

int voltage_to_adc(float voltage) {
  float safe_voltage = clamp_float(voltage, 0.0f, ADC_REFERENCE_VOLTAGE);
  return (int)((safe_voltage / ADC_REFERENCE_VOLTAGE) * 4095.0f);
}

float read_ph_from_adc(int adc_raw) {
  float voltage = adc_to_voltage(adc_raw);
  if (isnan(voltage) || PH_SLOPE_VOLT_PER_PH <= 0.0001f) {
    return NAN;
  }

  float ph = 7.0f + ((PH_NEUTRAL_VOLTAGE - voltage) / PH_SLOPE_VOLT_PER_PH);
  return clamp_float(ph, PH_MIN_VALUE, PH_MAX_VALUE);
}

float read_do_from_adc(int adc_raw) {
  float voltage = adc_to_voltage(adc_raw);
  float span = DO_FULL_SCALE_VOLTAGE - DO_ZERO_VOLTAGE;
  if (isnan(voltage) || span <= 0.0001f) {
    return NAN;
  }

  float normalized = (voltage - DO_ZERO_VOLTAGE) / span;
  float do_value = normalized * DO_FULL_SCALE_MG_L;
  return clamp_float(do_value, 0.0f, DO_MAX_VALUE);
}

float read_co2_from_adc(int adc_raw) {
  float voltage = adc_to_voltage(adc_raw);
  float span = CO2_FULL_SCALE_VOLTAGE - CO2_ZERO_VOLTAGE;
  if (isnan(voltage) || span <= 0.0001f) {
    return NAN;
  }

  float normalized = (voltage - CO2_ZERO_VOLTAGE) / span;
  float co2 = normalized * CO2_FULL_SCALE_PPM;
  return clamp_float(co2, 0.0f, CO2_MAX_VALUE);
}

float read_turbidity_from_adc(int adc_raw) {
  float voltage = adc_to_voltage(adc_raw);
  if (isnan(voltage)) {
    return NAN;
  }

  if (!TURBIDITY_CALIBRATED) {
    return (float)adc_raw;
  }

  float span = TURBIDITY_ZERO_NTU_VOLTAGE - TURBIDITY_MAX_NTU_VOLTAGE;
  if (span <= 0.0001f) {
    return NAN;
  }

  float normalized = (TURBIDITY_ZERO_NTU_VOLTAGE - voltage) / span;
  float ntu = normalized * TURBIDITY_MAX_NTU;
  return clamp_float(ntu, 0.0f, TURBIDITY_MAX_NTU);
}

float read_water_level_from_adc(int adc_raw) {
  float span = (float)(WATER_LEVEL_ADC_FULL - WATER_LEVEL_ADC_EMPTY);
  if (adc_raw < 0 || span == 0.0f) {
    return NAN;
  }

  float normalized = ((float)adc_raw - (float)WATER_LEVEL_ADC_EMPTY) / span;
  float percent = WATER_LEVEL_PERCENT_EMPTY + normalized * (WATER_LEVEL_PERCENT_FULL - WATER_LEVEL_PERCENT_EMPTY);
  return clamp_float(percent, 0.0f, 100.0f);
}

float read_current_from_adc(int adc_raw, float zero_voltage, float amp_per_volt, float max_value) {
  float voltage = adc_to_voltage(adc_raw);
  if (isnan(voltage) || amp_per_volt <= 0.0f) {
    return NAN;
  }

  float current = (voltage - zero_voltage) * amp_per_volt;
  if (current < 0.0f) {
    current = 0.0f;
  }
  return clamp_float(current, 0.0f, max_value);
}

bool has_water_level_runtime_data() {
  return WATER_LEVEL_SENSOR_ENABLED &&
         is_adc1_pin(WATER_LEVEL_PIN) &&
         (WATER_LEVEL_ADC_FULL != WATER_LEVEL_ADC_EMPTY);
}

bool has_co2_runtime_data() {
  return CO2_SENSOR_ENABLED && is_adc1_pin(CO2_PIN);
}

bool has_required_analog_runtime_data() {
  return is_adc1_pin(PH_PIN) && is_adc1_pin(DO_PIN) && is_adc1_pin(TURBIDITY_PIN);
}

bool has_aerator_current_runtime_data() {
  return AERATOR_CURRENT_SENSOR_ENABLED &&
         is_adc1_pin(AERATOR_CURRENT_PIN);
}

bool has_pump_current_runtime_data() {
  return PUMP_CURRENT_SENSOR_ENABLED &&
         is_adc1_pin(PUMP_CURRENT_PIN);
}

bool publish_float_topic(const char* topic, float value) {
  if (isnan(value)) {
    return false;
  }

  mqtt_client.publish(topic, String(value, 2).c_str(), true);
  return true;
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  initialize_device_identity();

  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");

  if (SENSOR_TEST_MODE) {
    strcpy(current_mode, "MANUAL");
  } else {
    strcpy(current_mode, DEFAULT_MODE);
  }
  strcpy(current_species, "Rô Phi");

  memset(&sensors, 0, sizeof(sensors));

  outputs.pump = false;
  outputs.aerator = false;
  outputs.aerator_2 = false;
  outputs.circulation = false;
  outputs.feeder = false;
  outputs.alarm_output = false;

  sensors.has_last_ph = false;
  sensors.ph_trend = BENCH_DEFAULT_PH_TREND;

  analogReadResolution(12);
  required_analog_pins_valid = has_required_analog_runtime_data();
  if (required_analog_pins_valid) {
    analogSetPinAttenuation(PH_PIN, ADC_11db);
    analogSetPinAttenuation(TURBIDITY_PIN, ADC_11db);
    analogSetPinAttenuation(DO_PIN, ADC_11db);
  } else if (!BENCH_TEST_MODE) {
    Serial.println("[CONFIG] Invalid analog pin mapping for PH/DO/TURBIDITY. Use ADC1 pins only.");
  }
  if (has_co2_runtime_data()) {
    analogSetPinAttenuation(CO2_PIN, ADC_11db);
  } else if (CO2_SENSOR_ENABLED) {
    Serial.println("[WARN] CO2 sensor enabled but CO2_PIN is not ADC1; sensor will be unavailable");
  }
  if (has_water_level_runtime_data()) {
    analogSetPinAttenuation(WATER_LEVEL_PIN, ADC_11db);
  }
  if (has_aerator_current_runtime_data()) {
    analogSetPinAttenuation(AERATOR_CURRENT_PIN, ADC_11db);
  }
  if (has_pump_current_runtime_data()) {
    analogSetPinAttenuation(PUMP_CURRENT_PIN, ADC_11db);
  }

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

  light_sensor_available = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (light_sensor_available) {
    Serial.println("[OK] BH1750 Light Sensor initialized");
  } else {
    Serial.println("[WARN] BH1750 Light Sensor not found (optional)");
  }

  if (SENSOR_TEST_MODE) {
    Serial.println("[INIT] SENSOR_TEST_MODE enabled - relays are locked OFF");
  } else if (BENCH_TEST_MODE) {
    Serial.println("[INIT] BENCH_TEST_MODE enabled - using simulated sensor values");
  }

  setup_wifi();

  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(4096);

  update_system_status_flags();

  Serial.println("===== Initialization Complete =====\n");
}

// ==================== MAIN LOOP ====================
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

  if (!SENSOR_TEST_MODE && now - last_rule_engine >= RULE_ENGINE_INTERVAL && strcmp(current_mode, "AUTO") == 0) {
    apply_species_rules();
    last_rule_engine = now;
  }

  if (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL) {
    if (mqtt_client.connected()) {
      update_system_status_flags();
      publish_sensor_data();
      publish_output_state();
      Serial.println("[MQTT] Data published successfully");
    }
    last_mqtt_publish = now;
  }

  delay(10);
}

// ==================== WIFI SETUP ====================
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

// ==================== MQTT RECONNECT ====================
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
  mqtt_connection_attempted = true;

  if (mqtt_client.connect(
        mqtt_runtime_client_id,
        MQTT_USER,
        MQTT_PASSWORD,
        MQTT_TOPIC_AVAILABILITY,
        0,
        true,
        "offline"
      )) {
    Serial.println("[OK] MQTT Connected!");
    mqtt_connected_once = true;

    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY, "online", true);
    mqtt_client.publish(MQTT_TOPIC_STATUS, "online", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_CO2, has_co2_runtime_data() ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_LIGHT, light_sensor_available ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_WATER_LEVEL, has_water_level_runtime_data() ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_AERATOR_CURRENT, has_aerator_current_runtime_data() ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_PUMP_CURRENT, has_pump_current_runtime_data() ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_AERATOR_2, AERATOR_2_HARDWARE_AVAILABLE ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_ALARM_OUTPUT, ALARM_OUTPUT_HARDWARE_AVAILABLE ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_TEST_MODE, SENSOR_TEST_MODE ? "SENSOR_TEST" : (BENCH_TEST_MODE ? "BENCH_TEST" : "PRODUCTION"), true);

    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_PUMP);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR_1);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR_2);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_CIRCULATION);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_FEEDER);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_ALARM_OUTPUT);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_MODE);
    mqtt_client.subscribe(MQTT_TOPIC_CONFIG_SPECIES);

    publish_mqtt_discovery();

    mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
    mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
    update_system_status_flags();
    publish_sensor_data();
    publish_output_state();
  } else {
    Serial.print("[WARN] MQTT connection failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

// ==================== HOME ASSISTANT MQTT DISCOVERY ====================
void publish_mqtt_discovery() {
  Serial.println("[HA Discovery] Publishing entity discoveries...");

  publish_sensor_discovery(
    "aquaculture_water_temp",
    "Aquaculture Water Temperature",
    MQTT_TOPIC_WATER_TEMP,
    "°C",
    "mdi:thermometer",
    "temperature",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_ph",
    "Aquaculture pH",
    MQTT_TOPIC_PH,
    "pH",
    "mdi:test-tube",
    "",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_ph_trend",
    "Aquaculture pH Trend",
    MQTT_TOPIC_PH_TREND,
    "pH/h",
    "mdi:chart-line",
    "",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_do",
    "Aquaculture Dissolved Oxygen",
    MQTT_TOPIC_DO,
    "mg/L",
    "mdi:water",
    "",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_water_level",
    "Aquaculture Water Level",
    MQTT_TOPIC_WATER_LEVEL,
    "%",
    "mdi:water-percent",
    "",
    "measurement",
    MQTT_TOPIC_AVAILABILITY_WATER_LEVEL,
    has_water_level_runtime_data()
  );

  publish_sensor_discovery(
    "aquaculture_turbidity",
    TURBIDITY_CALIBRATED ? "Aquaculture Turbidity" : "Aquaculture Turbidity Raw ADC",
    MQTT_TOPIC_TURBIDITY,
    TURBIDITY_CALIBRATED ? "NTU" : "adc",
    "mdi:water-opacity",
    "",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_air_temp",
    "Aquaculture Air Temperature",
    MQTT_TOPIC_AIR_TEMP,
    "°C",
    "mdi:thermometer",
    "temperature",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_humidity",
    "Aquaculture Humidity",
    MQTT_TOPIC_HUMIDITY,
    "%",
    "mdi:water-percent",
    "humidity",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_light",
    "Aquaculture Light Level",
    MQTT_TOPIC_LIGHT,
    "lux",
    "mdi:brightness-6",
    "illuminance",
    "measurement",
    MQTT_TOPIC_AVAILABILITY_LIGHT,
    light_sensor_available
  );

  publish_sensor_discovery(
    "aquaculture_co2",
    "Aquaculture CO2",
    MQTT_TOPIC_CO2,
    "ppm",
    "mdi:molecule-co2",
    "carbon_dioxide",
    "measurement",
    MQTT_TOPIC_AVAILABILITY_CO2,
    has_co2_runtime_data()
  );

  publish_sensor_discovery(
    "aquaculture_aerator_current",
    "Aquaculture Aerator Current",
    MQTT_TOPIC_AERATOR_CURRENT,
    "A",
    "mdi:current-ac",
    "current",
    "measurement",
    MQTT_TOPIC_AVAILABILITY_AERATOR_CURRENT,
    has_aerator_current_runtime_data()
  );

  publish_sensor_discovery(
    "aquaculture_pump_current",
    "Aquaculture Pump Current",
    MQTT_TOPIC_PUMP_CURRENT,
    "A",
    "mdi:current-ac",
    "current",
    "measurement",
    MQTT_TOPIC_AVAILABILITY_PUMP_CURRENT,
    has_pump_current_runtime_data()
  );

  publish_sensor_discovery(
    "aquaculture_controller_state",
    "Aquaculture Controller State",
    MQTT_TOPIC_STATE,
    "",
    "mdi:chip",
    "",
    "",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_controller_status",
    "Aquaculture Controller Status",
    MQTT_TOPIC_CONTROLLER_STATUS,
    "",
    "mdi:information-outline",
    "",
    "",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_test_mode",
    "Aquaculture Test Mode",
    MQTT_TOPIC_TEST_MODE,
    "",
    "mdi:test-tube",
    "",
    "",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_safety_state",
    "Aquaculture Safety State",
    MQTT_TOPIC_SAFETY_STATE,
    "",
    "mdi:shield-check",
    "",
    "",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_alarm_text",
    "Aquaculture Alarm Text",
    MQTT_TOPIC_ALARM_TEXT,
    "",
    "mdi:alarm-light",
    "",
    "",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_binary_sensor_discovery(
    "aquaculture_alarm_active",
    "Aquaculture Alarm Active",
    MQTT_TOPIC_ALARM_ACTIVE,
    "mdi:alarm-light",
    "problem",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_binary_sensor_discovery(
    "aquaculture_safety_active",
    "Aquaculture Safety Active",
    MQTT_TOPIC_SAFETY_ACTIVE,
    "mdi:shield-alert",
    "safety",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_binary_sensor_discovery(
    "aquaculture_emergency_active",
    "Aquaculture Emergency Active",
    MQTT_TOPIC_EMERGENCY_ACTIVE,
    "mdi:alert-octagon",
    "problem",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_switch_discovery(
    "aquaculture_pump",
    "Aquaculture Pump",
    MQTT_TOPIC_PUMP,
    MQTT_TOPIC_CONTROL_PUMP,
    "mdi:pump",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_switch_discovery(
    "aquaculture_aerator",
    "Aquaculture Aerator",
    MQTT_TOPIC_AERATOR,
    MQTT_TOPIC_CONTROL_AERATOR,
    "mdi:air-purifier",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_switch_discovery(
    "aquaculture_aerator_1",
    "Aquaculture Aerator 1",
    MQTT_TOPIC_AERATOR_1,
    MQTT_TOPIC_CONTROL_AERATOR_1,
    "mdi:air-purifier",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_switch_discovery(
    "aquaculture_aerator_2",
    "Aquaculture Aerator 2",
    MQTT_TOPIC_AERATOR_2,
    MQTT_TOPIC_CONTROL_AERATOR_2,
    "mdi:air-purifier",
    MQTT_TOPIC_AVAILABILITY_AERATOR_2,
    AERATOR_2_HARDWARE_AVAILABLE
  );

  publish_switch_discovery(
    "aquaculture_circulation",
    "Aquaculture Circulation",
    MQTT_TOPIC_CIRCULATION,
    MQTT_TOPIC_CONTROL_CIRCULATION,
    "mdi:water-pump",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_switch_discovery(
    "aquaculture_feeder",
    "Aquaculture Feeder",
    MQTT_TOPIC_FEEDER,
    MQTT_TOPIC_CONTROL_FEEDER,
    "mdi:fish-food",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_switch_discovery(
    "aquaculture_alarm_output",
    "Aquaculture Alarm Output",
    MQTT_TOPIC_ALARM_OUTPUT,
    MQTT_TOPIC_CONTROL_ALARM_OUTPUT,
    "mdi:alarm-bell",
    MQTT_TOPIC_AVAILABILITY_ALARM_OUTPUT,
    ALARM_OUTPUT_HARDWARE_AVAILABLE
  );

  {
    StaticJsonDocument<1024> doc;
    doc["name"] = "Aquaculture Mode";
    doc["unique_id"] = build_unique_id("aquaculture_mode");
    doc["state_topic"] = MQTT_TOPIC_MODE_STATE;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_MODE;
    doc["icon"] = "mdi:cog";
    doc["options"][0] = "AUTO";
    doc["options"][1] = "MANUAL";
    doc["options"][2] = "SCHEDULE";
    doc["options"][3] = "SAFE";
    set_common_availability(doc, MQTT_TOPIC_AVAILABILITY);
    attach_device_metadata(doc);
    publish_discovery_payload("select", "aquaculture_mode", doc);
  }

  {
    StaticJsonDocument<1280> doc;
    doc["name"] = "Aquaculture Species";
    doc["unique_id"] = build_unique_id("aquaculture_species");
    doc["state_topic"] = MQTT_TOPIC_SPECIES_STATE;
    doc["command_topic"] = MQTT_TOPIC_CONFIG_SPECIES;
    doc["icon"] = "mdi:fish";
    doc["options"][0] = "Koi";
    doc["options"][1] = "Cá Trắm";
    doc["options"][2] = "Cá Chép";
    doc["options"][3] = "Cá Tra";
    doc["options"][4] = "Cá Lóc";
    doc["options"][5] = "Tôm Thẻ";
    doc["options"][6] = "Tôm Sú";
    doc["options"][7] = "Rô Phi";
    doc["options"][8] = "Tilapia";
    set_common_availability(doc, MQTT_TOPIC_AVAILABILITY);
    attach_device_metadata(doc);
    publish_discovery_payload("select", "aquaculture_species", doc);
  }

  Serial.println("[OK] All discoveries published!");
}

void set_output(const char* name, bool requested_state) {
  bool hardware_available = true;

  if (strcmp(name, "aerator_2") == 0) {
    hardware_available = AERATOR_2_HARDWARE_AVAILABLE;
  } else if (strcmp(name, "alarm_output") == 0) {
    hardware_available = ALARM_OUTPUT_HARDWARE_AVAILABLE;
  }

  bool effective_state = requested_state;

  if (requested_state && outputs_locked_for_test()) {
    effective_state = false;
    Serial.print("[TEST] Suppressed ON command for ");
    Serial.println(name);
  } else if (requested_state && !hardware_available) {
    effective_state = false;
    Serial.print("[OUTPUT] Hardware unavailable for ");
    Serial.print(name);
    Serial.println("; forcing OFF");
  }

  if (strcmp(name, "pump") == 0) {
    outputs.pump = effective_state;
    digitalWrite(PUMP_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "aerator") == 0 || strcmp(name, "aerator_1") == 0) {
    outputs.aerator = effective_state;
    digitalWrite(AERATOR_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "aerator_2") == 0) {
    outputs.aerator_2 = effective_state;
  } else if (strcmp(name, "circulation") == 0) {
    outputs.circulation = effective_state;
    digitalWrite(CIRCULATION_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "feeder") == 0) {
    outputs.feeder = effective_state;
    digitalWrite(FEEDER_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "alarm_output") == 0) {
    outputs.alarm_output = effective_state;
  }

  update_system_status_flags();

  Serial.print("[OUTPUT] ");
  Serial.print(name);
  Serial.println(effective_state ? " ON" : " OFF");
}

// ==================== MQTT CALLBACK ====================
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("[MQTT] Message received on: ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(message);

  bool requested_on = message.equalsIgnoreCase("ON");
  bool is_output_command =
    strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0 ||
    strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR) == 0 ||
    strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR_1) == 0 ||
    strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR_2) == 0 ||
    strcmp(topic, MQTT_TOPIC_CONTROL_CIRCULATION) == 0 ||
    strcmp(topic, MQTT_TOPIC_CONTROL_FEEDER) == 0 ||
    strcmp(topic, MQTT_TOPIC_CONTROL_ALARM_OUTPUT) == 0;

  if (is_output_command && strcmp(current_mode, "MANUAL") != 0) {
    strncpy(current_mode, "MANUAL", sizeof(current_mode) - 1);
    current_mode[sizeof(current_mode) - 1] = '\0';
    update_system_status_flags();
    mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
    mqtt_client.publish(MQTT_TOPIC_STATE, current_mode, true);
    publish_runtime_status_topics();
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0) {
    set_output("pump", requested_on);
    mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR) == 0) {
    set_output("aerator", requested_on);
    mqtt_client.publish(MQTT_TOPIC_AERATOR, outputs.aerator ? "ON" : "OFF", true);
    mqtt_client.publish(MQTT_TOPIC_AERATOR_1, outputs.aerator ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR_1) == 0) {
    set_output("aerator_1", requested_on);
    mqtt_client.publish(MQTT_TOPIC_AERATOR, outputs.aerator ? "ON" : "OFF", true);
    mqtt_client.publish(MQTT_TOPIC_AERATOR_1, outputs.aerator ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR_2) == 0) {
    set_output("aerator_2", requested_on);
    mqtt_client.publish(MQTT_TOPIC_AERATOR_2, outputs.aerator_2 ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_CIRCULATION) == 0) {
    set_output("circulation", requested_on);
    mqtt_client.publish(MQTT_TOPIC_CIRCULATION, outputs.circulation ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_FEEDER) == 0) {
    set_output("feeder", requested_on);
    mqtt_client.publish(MQTT_TOPIC_FEEDER, outputs.feeder ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_ALARM_OUTPUT) == 0) {
    set_output("alarm_output", requested_on);
    mqtt_client.publish(MQTT_TOPIC_ALARM_OUTPUT, outputs.alarm_output ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_MODE) == 0) {
    if (SENSOR_TEST_MODE) {
      if (!message.equalsIgnoreCase("MANUAL")) {
        Serial.print("[TEST] Ignored mode change while SENSOR_TEST_MODE active: ");
        Serial.println(message);
      }
      strncpy(current_mode, "MANUAL", sizeof(current_mode) - 1);
      current_mode[sizeof(current_mode) - 1] = '\0';
      update_system_status_flags();
      mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
      mqtt_client.publish(MQTT_TOPIC_STATE, current_mode, true);
      publish_runtime_status_topics();
      return;
    }

    if (is_supported_mode(message.c_str())) {
      strncpy(current_mode, message.c_str(), sizeof(current_mode) - 1);
      current_mode[sizeof(current_mode) - 1] = '\0';
      Serial.print("[CONFIG] Mode changed to: ");
      Serial.println(current_mode);
      update_system_status_flags();
      mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
      mqtt_client.publish(MQTT_TOPIC_STATE, current_mode, true);
      publish_runtime_status_topics();
    } else {
      Serial.print("[CONFIG] Ignored unsupported mode: ");
      Serial.println(message);
      mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
    }
  }

  if (strcmp(topic, MQTT_TOPIC_CONFIG_SPECIES) == 0) {
    if (is_supported_species(message.c_str())) {
      strncpy(current_species, message.c_str(), sizeof(current_species) - 1);
      current_species[sizeof(current_species) - 1] = '\0';
      Serial.print("[CONFIG] Species changed to: ");
      Serial.println(current_species);
      update_system_status_flags();
      mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
      publish_runtime_status_topics();
    } else {
      Serial.print("[CONFIG] Ignored unsupported species: ");
      Serial.println(message);
      mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
    }
  }
}

void publish_runtime_status_topics() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_STATE, current_mode, true);
  mqtt_client.publish(MQTT_TOPIC_CONTROLLER_STATUS, controller_status, true);
  mqtt_client.publish(MQTT_TOPIC_SAFETY_STATE, safety_state, true);
  mqtt_client.publish(MQTT_TOPIC_ALARM_TEXT, alarm_text, true);
  mqtt_client.publish(MQTT_TOPIC_ALARM_ACTIVE, alarm_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_SAFETY_ACTIVE, safety_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_EMERGENCY_ACTIVE, emergency_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_TEST_MODE, SENSOR_TEST_MODE ? "SENSOR_TEST" : (BENCH_TEST_MODE ? "BENCH_TEST" : "PRODUCTION"), true);
}

void update_system_status_flags() {
  if (!required_analog_pins_valid && !BENCH_TEST_MODE) {
    alarm_active = true;
    emergency_active = false;
    safety_active = true;
    snprintf(safety_state, sizeof(safety_state), "ALARM");
    snprintf(controller_status, sizeof(controller_status), "CONFIG_ERROR_ADC_PIN");
    snprintf(alarm_text, sizeof(alarm_text), "Invalid ADC pin mapping for required analog sensors");
    publish_runtime_status_topics();
    return;
  }

  if (SENSOR_TEST_MODE) {
    alarm_active = false;
    emergency_active = false;
    safety_active = false;
    snprintf(safety_state, sizeof(safety_state), "SENSOR_TEST");
    snprintf(alarm_text, sizeof(alarm_text), "Sensor test mode: outputs locked");
    snprintf(controller_status, sizeof(controller_status), "SENSOR_TEST_RUNNING");
    publish_runtime_status_topics();
    return;
  }

  const SpeciesRule* rule = getSpeciesRule(current_species);

  alarm_active =
    (sensors.do_value <= rule->do_critical && sensors.do_value > 0) ||
    (sensors.water_temp >= rule->temp_critical_high) ||
    (sensors.ph > rule->ph_max && sensors.ph > 0) ||
    (sensors.ph < rule->ph_min && sensors.ph > 0) ||
    outputs.alarm_output;

  emergency_active =
    (sensors.do_value <= rule->do_critical && sensors.do_value > 0) ||
    (sensors.water_temp >= rule->temp_critical_high);

  safety_active = alarm_active || emergency_active;

  if (emergency_active) {
    snprintf(safety_state, sizeof(safety_state), "EMERGENCY");
    snprintf(alarm_text, sizeof(alarm_text), "Critical DO or temperature");
  } else if (alarm_active) {
    snprintf(safety_state, sizeof(safety_state), "ALARM");
    snprintf(alarm_text, sizeof(alarm_text), "Threshold alert active");
  } else {
    snprintf(safety_state, sizeof(safety_state), "NORMAL");
    snprintf(alarm_text, sizeof(alarm_text), "No alarms");
  }

  if (!mqtt_connection_attempted && !mqtt_connected_once) {
    snprintf(controller_status, sizeof(controller_status), "INITIALIZING");
  } else if (mqtt_client.connected()) {
    snprintf(controller_status, sizeof(controller_status), "RUNNING");
  } else {
    snprintf(controller_status, sizeof(controller_status), "DISCONNECTED");
  }

  publish_runtime_status_topics();
}

// ==================== READ SENSORS ====================
void read_sensors() {
  if (BENCH_TEST_MODE) {
    sensors.water_temp = BENCH_DEFAULT_WATER_TEMP;
    sensors.air_temp = BENCH_DEFAULT_AIR_TEMP;
    sensors.air_humidity = BENCH_DEFAULT_AIR_HUMIDITY;
    sensors.ph = BENCH_DEFAULT_PH;
    sensors.ph_trend = BENCH_DEFAULT_PH_TREND;
    sensors.turbidity = BENCH_DEFAULT_TURBIDITY;
    sensors.do_value = BENCH_DEFAULT_DO;
    sensors.co2 = BENCH_DEFAULT_CO2;
    sensors.light = BENCH_DEFAULT_LIGHT;
    sensors.water_level = has_water_level_runtime_data() ? BENCH_DEFAULT_WATER_LEVEL : 0.0f;
    sensors.aerator_current = has_aerator_current_runtime_data() ? BENCH_DEFAULT_AERATOR_CURRENT : 0.0f;
    sensors.pump_current = has_pump_current_runtime_data() ? BENCH_DEFAULT_PUMP_CURRENT : 0.0f;
    float bench_ph_voltage = PH_NEUTRAL_VOLTAGE - ((BENCH_DEFAULT_PH - 7.0f) * PH_SLOPE_VOLT_PER_PH);
    float do_span = DO_FULL_SCALE_VOLTAGE - DO_ZERO_VOLTAGE;
    float bench_do_ratio = DO_FULL_SCALE_MG_L > 0.0f ? (BENCH_DEFAULT_DO / DO_FULL_SCALE_MG_L) : 0.0f;
    float bench_do_voltage = DO_ZERO_VOLTAGE + (bench_do_ratio * do_span);
    sensors.ph_raw = voltage_to_adc(bench_ph_voltage);
    sensors.do_raw = voltage_to_adc(bench_do_voltage);
    if (TURBIDITY_CALIBRATED && TURBIDITY_MAX_NTU > 0.0f) {
      float bench_turbidity_voltage =
        TURBIDITY_ZERO_NTU_VOLTAGE -
        ((BENCH_DEFAULT_TURBIDITY / TURBIDITY_MAX_NTU) * (TURBIDITY_ZERO_NTU_VOLTAGE - TURBIDITY_MAX_NTU_VOLTAGE));
      sensors.turbidity_raw = voltage_to_adc(bench_turbidity_voltage);
    } else {
      sensors.turbidity_raw = (int)BENCH_DEFAULT_TURBIDITY;
    }
    if (has_water_level_runtime_data()) {
      float water_level_span = WATER_LEVEL_PERCENT_FULL - WATER_LEVEL_PERCENT_EMPTY;
      float water_level_ratio = water_level_span != 0.0f
                                  ? (BENCH_DEFAULT_WATER_LEVEL - WATER_LEVEL_PERCENT_EMPTY) / water_level_span
                                  : 0.0f;
      water_level_ratio = clamp_float(water_level_ratio, 0.0f, 1.0f);
      sensors.water_level_raw =
        WATER_LEVEL_ADC_EMPTY + (int)(water_level_ratio * (float)(WATER_LEVEL_ADC_FULL - WATER_LEVEL_ADC_EMPTY));
    } else {
      sensors.water_level_raw = -1;
    }

    if (has_aerator_current_runtime_data() && AERATOR_CURRENT_AMP_PER_VOLT > 0.0f) {
      float aerator_current_voltage = AERATOR_CURRENT_ZERO_VOLTAGE + (BENCH_DEFAULT_AERATOR_CURRENT / AERATOR_CURRENT_AMP_PER_VOLT);
      sensors.aerator_current_raw = voltage_to_adc(aerator_current_voltage);
    } else {
      sensors.aerator_current_raw = -1;
    }

    if (has_pump_current_runtime_data() && PUMP_CURRENT_AMP_PER_VOLT > 0.0f) {
      float pump_current_voltage = PUMP_CURRENT_ZERO_VOLTAGE + (BENCH_DEFAULT_PUMP_CURRENT / PUMP_CURRENT_AMP_PER_VOLT);
      sensors.pump_current_raw = voltage_to_adc(pump_current_voltage);
    } else {
      sensors.pump_current_raw = -1;
    }
    sensors.ph_voltage = adc_to_voltage(sensors.ph_raw);
    sensors.do_voltage = adc_to_voltage(sensors.do_raw);
    sensors.turbidity_voltage = adc_to_voltage(sensors.turbidity_raw);
  } else {
    waterTemp.requestTemperatures();
    sensors.water_temp = waterTemp.getTempCByIndex(0);
    if (sensors.water_temp <= -100.0f || sensors.water_temp > 125.0f) sensors.water_temp = NAN;

    sensors.air_temp = dht.readTemperature();
    sensors.air_humidity = dht.readHumidity();
    if (isnan(sensors.air_temp)) sensors.air_temp = NAN;
    if (isnan(sensors.air_humidity)) sensors.air_humidity = NAN;

    if (light_sensor_available) {
      sensors.light = lightMeter.readLightLevel();
      if (sensors.light < 0) sensors.light = NAN;
    } else {
      sensors.light = NAN;
    }

    if (required_analog_pins_valid) {
      sensors.ph_raw = read_analog_average(PH_PIN, ANALOG_READ_SAMPLES);
      sensors.do_raw = read_analog_average(DO_PIN, ANALOG_READ_SAMPLES);
      sensors.turbidity_raw = read_analog_average(TURBIDITY_PIN, ANALOG_READ_SAMPLES);
      sensors.ph_voltage = adc_to_voltage(sensors.ph_raw);
      sensors.do_voltage = adc_to_voltage(sensors.do_raw);
      sensors.turbidity_voltage = adc_to_voltage(sensors.turbidity_raw);

      sensors.ph = read_ph_from_adc(sensors.ph_raw);
      sensors.do_value = read_do_from_adc(sensors.do_raw);
      sensors.turbidity = read_turbidity_from_adc(sensors.turbidity_raw);
    } else {
      sensors.ph_raw = -1;
      sensors.do_raw = -1;
      sensors.turbidity_raw = -1;
      sensors.ph_voltage = NAN;
      sensors.do_voltage = NAN;
      sensors.turbidity_voltage = NAN;
      sensors.ph = NAN;
      sensors.do_value = NAN;
      sensors.turbidity = NAN;
    }
    sensors.co2 = has_co2_runtime_data() ? read_co2_from_adc(read_analog_average(CO2_PIN, ANALOG_READ_SAMPLES)) : NAN;

    sensors.water_level_raw = -1;
    sensors.aerator_current_raw = -1;
    sensors.pump_current_raw = -1;
    sensors.water_level = NAN;
    sensors.aerator_current = NAN;
    sensors.pump_current = NAN;

    if (has_water_level_runtime_data()) {
      sensors.water_level_raw = read_analog_average(WATER_LEVEL_PIN, ANALOG_READ_SAMPLES);
      sensors.water_level = read_water_level_from_adc(sensors.water_level_raw);
    }
    if (has_aerator_current_runtime_data()) {
      sensors.aerator_current_raw = read_analog_average(AERATOR_CURRENT_PIN, ANALOG_READ_SAMPLES);
      sensors.aerator_current = read_current_from_adc(
        sensors.aerator_current_raw,
        AERATOR_CURRENT_ZERO_VOLTAGE,
        AERATOR_CURRENT_AMP_PER_VOLT,
        AERATOR_CURRENT_MAX_VALUE
      );
    }
    if (has_pump_current_runtime_data()) {
      sensors.pump_current_raw = read_analog_average(PUMP_CURRENT_PIN, ANALOG_READ_SAMPLES);
      sensors.pump_current = read_current_from_adc(
        sensors.pump_current_raw,
        PUMP_CURRENT_ZERO_VOLTAGE,
        PUMP_CURRENT_AMP_PER_VOLT,
        PUMP_CURRENT_MAX_VALUE
      );
    }

    uint32_t now = millis();
    if (!isnan(sensors.ph) && sensors.has_last_ph && now > sensors.last_ph_timestamp) {
      float hours = (now - sensors.last_ph_timestamp) / 3600000.0f;
      if (hours > 0.0f) {
        sensors.ph_trend = (sensors.ph - sensors.last_ph) / hours;
      } else {
        sensors.ph_trend = 0.0f;
      }
    } else {
      sensors.ph_trend = 0.0f;
    }

    if (!isnan(sensors.ph)) {
      sensors.last_ph = sensors.ph;
      sensors.last_ph_timestamp = now;
      sensors.has_last_ph = true;
    } else {
      sensors.last_ph = 0.0f;
      sensors.last_ph_timestamp = 0;
      sensors.has_last_ph = false;
      sensors.ph_trend = 0.0f;
    }
  }

  sensors.last_read = millis();
  update_system_status_flags();

  Serial.println("===== SENSOR READINGS =====");
  Serial.print("Water Temp: ");
  Serial.print(sensors.water_temp, 2);
  Serial.println("°C");
  Serial.print("pH: ");
  Serial.println(sensors.ph, 2);
  Serial.print("pH raw/volt: ");
  Serial.print(sensors.ph_raw);
  Serial.print(" / ");
  Serial.println(sensors.ph_voltage, 3);
  Serial.print("pH Trend: ");
  Serial.print(sensors.ph_trend, 2);
  Serial.println(" pH/h");
  Serial.print("DO: ");
  Serial.print(sensors.do_value, 2);
  Serial.println(" mg/L");
  Serial.print("DO raw/volt: ");
  Serial.print(sensors.do_raw);
  Serial.print(" / ");
  Serial.println(sensors.do_voltage, 3);
  Serial.print("CO2: ");
  Serial.print(sensors.co2, 2);
  Serial.println(" ppm");
  Serial.print("Water Level: ");
  Serial.print(sensors.water_level, 2);
  Serial.println(" %");
  Serial.print("Turbidity: ");
  Serial.println(sensors.turbidity, 2);
  Serial.print("Turbidity raw/volt: ");
  Serial.print(sensors.turbidity_raw);
  Serial.print(" / ");
  Serial.println(sensors.turbidity_voltage, 3);
  Serial.print("Air Temp: ");
  Serial.print(sensors.air_temp, 2);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(sensors.air_humidity, 2);
  Serial.println("%");
  Serial.print("Light: ");
  Serial.print(sensors.light, 2);
  Serial.println(" lux");
  if (has_water_level_runtime_data()) {
    Serial.print("Water Level raw: ");
    Serial.println(sensors.water_level_raw);
  }
  if (has_aerator_current_runtime_data()) {
    Serial.print("Aerator Current: ");
    Serial.print(sensors.aerator_current, 2);
    Serial.print(" A (raw ");
    Serial.print(sensors.aerator_current_raw);
    Serial.println(")");
  }
  if (has_pump_current_runtime_data()) {
    Serial.print("Pump Current: ");
    Serial.print(sensors.pump_current, 2);
    Serial.print(" A (raw ");
    Serial.print(sensors.pump_current_raw);
    Serial.println(")");
  }
}

// ==================== SPECIES-BASED RULE ENGINE ====================
void apply_species_rules() {
  const SpeciesRule* rule = getSpeciesRule(current_species);

  bool pump_on = outputs.pump;
  bool aerator_on = outputs.aerator;
  bool circulation_on = outputs.circulation;
  bool feeder_on = outputs.feeder;

  bool alert_temp_high = false;
  bool alert_temp_low = false;
  bool alert_ph = false;
  bool alert_do_low = false;
  bool alert_do_critical = false;
  bool alert_co2 = false;
  bool alert_turbidity = false;

  if (sensors.water_temp < -20 || sensors.water_temp > 60) sensors.water_temp = 0;
  if (sensors.air_temp < -20 || sensors.air_temp > 60) sensors.air_temp = 0;
  if (sensors.air_humidity < 0 || sensors.air_humidity > 100) sensors.air_humidity = 0;
  if (sensors.ph < 0 || sensors.ph > 14) sensors.ph = 0;
  if (sensors.do_value < 0 || sensors.do_value > 20) sensors.do_value = 0;
  if (sensors.co2 < 0 || sensors.co2 > 100) sensors.co2 = 0;
  if (sensors.light < 0) sensors.light = 0;
  if (sensors.turbidity < 0) sensors.turbidity = 0;

  if (sensors.water_temp > rule->temp_max) alert_temp_high = true;
  if (sensors.water_temp < rule->temp_min) alert_temp_low = true;
  if (sensors.ph < rule->ph_min || sensors.ph > rule->ph_max) alert_ph = true;
  if (sensors.do_value < rule->do_min) alert_do_low = true;
  if (sensors.do_value <= rule->do_critical) alert_do_critical = true;
  if (sensors.co2 > rule->co2_max) alert_co2 = true;
  if (sensors.turbidity > rule->turbidity_max) alert_turbidity = true;

  if (alert_do_critical) {
    aerator_on = true;
    pump_on = true;
  }

  if (sensors.water_temp >= rule->temp_critical_high) {
    circulation_on = true;
    pump_on = true;
  }

  if (alert_do_low) {
    aerator_on = true;
  }

  if (alert_temp_high) {
    circulation_on = true;
  }

  if (alert_co2) {
    pump_on = true;
    aerator_on = true;
  }

  if (sensors.do_value < rule->do_min + 0.5f) {
    circulation_on = true;
  }

  bool water_stable =
    !alert_do_low &&
    !alert_do_critical &&
    !alert_temp_high &&
    !alert_temp_low &&
    !alert_ph &&
    !alert_co2 &&
    !alert_turbidity;

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

  if (alert_do_critical || alert_temp_high || alert_ph || alert_co2 || alert_turbidity) {
    Serial.println("[RULES] Species-based control activated with alerts");
  }

  Serial.print("[RULES] Active species: ");
  Serial.println(rule->name);

  (void)water_stable;
}

// ==================== PUBLISH SENSOR DATA ====================
void publish_sensor_data() {
  if (!mqtt_client.connected()) {
    return;
  }

  publish_float_topic(MQTT_TOPIC_WATER_TEMP, sensors.water_temp);
  publish_float_topic(MQTT_TOPIC_PH, sensors.ph);
  publish_float_topic(MQTT_TOPIC_PH_TREND, sensors.ph_trend);
  publish_float_topic(MQTT_TOPIC_DO, sensors.do_value);
  publish_float_topic(MQTT_TOPIC_CO2, sensors.co2);
  publish_float_topic(MQTT_TOPIC_TURBIDITY, sensors.turbidity);
  publish_float_topic(MQTT_TOPIC_AIR_TEMP, sensors.air_temp);
  publish_float_topic(MQTT_TOPIC_HUMIDITY, sensors.air_humidity);
  publish_float_topic(MQTT_TOPIC_LIGHT, sensors.light);
  if (has_water_level_runtime_data()) {
    publish_float_topic(MQTT_TOPIC_WATER_LEVEL, sensors.water_level);
  }
  if (has_aerator_current_runtime_data()) {
    publish_float_topic(MQTT_TOPIC_AERATOR_CURRENT, sensors.aerator_current);
  }
  if (has_pump_current_runtime_data()) {
    publish_float_topic(MQTT_TOPIC_PUMP_CURRENT, sensors.pump_current);
  }

  publish_runtime_status_topics();
}

// ==================== PUBLISH OUTPUT STATE ====================
void publish_output_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR, outputs.aerator ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR_1, outputs.aerator ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR_2, outputs.aerator_2 ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_CIRCULATION, outputs.circulation ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_FEEDER, outputs.feeder ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_ALARM_OUTPUT, outputs.alarm_output ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
  mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
}
