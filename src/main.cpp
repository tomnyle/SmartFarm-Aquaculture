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
    strcmp(species, "Tilapia") == 0;
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  initialize_device_identity();

  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");

  strcpy(current_mode, DEFAULT_MODE);
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

  if (now - last_rule_engine >= RULE_ENGINE_INTERVAL && strcmp(current_mode, "AUTO") == 0) {
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

    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY, "online", true);
    mqtt_client.publish(MQTT_TOPIC_STATUS, "online", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_CO2, CO2_SENSOR_ENABLED ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_AERATOR_2, AERATOR_2_HARDWARE_AVAILABLE ? "online" : "offline", true);
    mqtt_client.publish(MQTT_TOPIC_AVAILABILITY_ALARM_OUTPUT, ALARM_OUTPUT_HARDWARE_AVAILABLE ? "online" : "offline", true);

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
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_turbidity",
    "Aquaculture Turbidity",
    MQTT_TOPIC_TURBIDITY,
    "NTU",
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
    MQTT_TOPIC_AVAILABILITY
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
    CO2_SENSOR_ENABLED
  );

  publish_sensor_discovery(
    "aquaculture_aerator_current",
    "Aquaculture Aerator Current",
    MQTT_TOPIC_AERATOR_CURRENT,
    "A",
    "mdi:current-ac",
    "current",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
  );

  publish_sensor_discovery(
    "aquaculture_pump_current",
    "Aquaculture Pump Current",
    MQTT_TOPIC_PUMP_CURRENT,
    "A",
    "mdi:current-ac",
    "current",
    "measurement",
    MQTT_TOPIC_AVAILABILITY
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
    doc["options"][7] = "Tilapia";
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

  if (requested_state && BENCH_TEST_MODE) {
    effective_state = false;
    Serial.print("[BENCH] Suppressed ON command for ");
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

  if (strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0) {
    strcpy(current_mode, "MANUAL");
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
    if (is_supported_mode(message.c_str())) {
      strncpy(current_mode, message.c_str(), sizeof(current_mode) - 1);
      current_mode[sizeof(current_mode) - 1] = '\0';
      Serial.print("[CONFIG] Mode changed to: ");
      Serial.println(current_mode);
      mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
      mqtt_client.publish(MQTT_TOPIC_STATE, current_mode, true);
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
      mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
    } else {
      Serial.print("[CONFIG] Ignored unsupported species: ");
      Serial.println(message);
      mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
    }
  }
}

void update_system_status_flags() {
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

  if (mqtt_client.connected()) {
    snprintf(controller_status, sizeof(controller_status), "RUNNING");
  } else {
    snprintf(controller_status, sizeof(controller_status), "DISCONNECTED");
  }
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
    sensors.water_level = BENCH_DEFAULT_WATER_LEVEL;
    sensors.aerator_current = BENCH_DEFAULT_AERATOR_CURRENT;
    sensors.pump_current = BENCH_DEFAULT_PUMP_CURRENT;
  } else {
    waterTemp.requestTemperatures();
    sensors.water_temp = waterTemp.getTempCByIndex(0);
    if (sensors.water_temp == -127) sensors.water_temp = 0;

    sensors.air_temp = dht.readTemperature();
    sensors.air_humidity = dht.readHumidity();
    if (isnan(sensors.air_temp)) sensors.air_temp = 0;
    if (isnan(sensors.air_humidity)) sensors.air_humidity = 0;

    if (light_sensor_available) {
      sensors.light = lightMeter.readLightLevel();
      if (sensors.light < 0) sensors.light = 0;
    } else {
      sensors.light = 0;
    }

    sensors.ph = (analogRead(PH_PIN) / 4095.0f) * 14.0f;
    sensors.turbidity = analogRead(TURBIDITY_PIN);
    sensors.do_value = (analogRead(DO_PIN) / 4095.0f) * 20.0f;
    sensors.co2 = CO2_SENSOR_ENABLED ? (analogRead(CO2_PIN) / 4095.0f) * 10.0f : 0.0f;

    sensors.water_level = 0.0f;
    sensors.aerator_current = 0.0f;
    sensors.pump_current = 0.0f;

    uint32_t now = millis();
    if (sensors.has_last_ph && now > sensors.last_ph_timestamp) {
      float hours = (now - sensors.last_ph_timestamp) / 3600000.0f;
      if (hours > 0.0f) {
        sensors.ph_trend = (sensors.ph - sensors.last_ph) / hours;
      } else {
        sensors.ph_trend = 0.0f;
      }
    } else {
      sensors.ph_trend = 0.0f;
    }

    sensors.last_ph = sensors.ph;
    sensors.last_ph_timestamp = now;
    sensors.has_last_ph = true;
  }

  sensors.last_read = millis();
  update_system_status_flags();

  Serial.println("===== SENSOR READINGS =====");
  Serial.print("Water Temp: ");
  Serial.print(sensors.water_temp, 2);
  Serial.println("°C");
  Serial.print("pH: ");
  Serial.println(sensors.ph, 2);
  Serial.print("pH Trend: ");
  Serial.print(sensors.ph_trend, 2);
  Serial.println(" pH/h");
  Serial.print("DO: ");
  Serial.print(sensors.do_value, 2);
  Serial.println(" mg/L");
  Serial.print("CO2: ");
  Serial.print(sensors.co2, 2);
  Serial.println(" ppm");
  Serial.print("Water Level: ");
  Serial.print(sensors.water_level, 2);
  Serial.println(" %");
  Serial.print("Turbidity: ");
  Serial.println(sensors.turbidity, 2);
  Serial.print("Air Temp: ");
  Serial.print(sensors.air_temp, 2);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(sensors.air_humidity, 2);
  Serial.println("%");
  Serial.print("Light: ");
  Serial.print(sensors.light, 2);
  Serial.println(" lux");
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

  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP, String(sensors.water_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH, String(sensors.ph, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH_TREND, String(sensors.ph_trend, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO, String(sensors.do_value, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_CO2, String(sensors.co2, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY, String(sensors.turbidity, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_AIR_TEMP, String(sensors.air_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY, String(sensors.air_humidity, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_LIGHT, String(sensors.light, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_WATER_LEVEL, String(sensors.water_level, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_AERATOR_CURRENT, String(sensors.aerator_current, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PUMP_CURRENT, String(sensors.pump_current, 2).c_str(), true);

  mqtt_client.publish(MQTT_TOPIC_STATE, current_mode, true);
  mqtt_client.publish(MQTT_TOPIC_CONTROLLER_STATUS, controller_status, true);
  mqtt_client.publish(MQTT_TOPIC_SAFETY_STATE, safety_state, true);
  mqtt_client.publish(MQTT_TOPIC_ALARM_TEXT, alarm_text, true);
  mqtt_client.publish(MQTT_TOPIC_ALARM_ACTIVE, alarm_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_SAFETY_ACTIVE, safety_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_EMERGENCY_ACTIVE, emergency_active ? "ON" : "OFF", true);
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
