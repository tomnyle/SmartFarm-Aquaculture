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

struct SensorValidity {
  bool water_temp;
  bool ph;
  bool do_value;
  bool turbidity;
  bool co2;
  bool required_ok;
};

struct ConditionState {
  bool water_temp_high;
  bool water_temp_low;
  bool ph_low;
  bool ph_high;
  bool do_low;
  bool do_critical;
  bool co2_high;
  bool turbidity_high;
  bool sensor_fault;
  bool any_active;
  bool outputs_locked;
};

// ==================== GLOBAL VARIABLES ====================
SensorData sensors;
OutputState outputs;
SensorValidity sensor_validity;
ConditionState conditions;

uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;
uint32_t last_pump_change = 0;
uint32_t last_aerator_change = 0;
uint32_t last_circulation_change = 0;
uint32_t last_feeder_change = 0;

char current_mode[16] = "";
char current_species[32] = "Rô Phi";
char production_phase[16] = "INIT";
char process_summary[96] = "init";
const char* relay_test_status = "NOT_STARTED";
bool production_ready = false;
bool monitor_state_initialized = false;
bool last_production_ready = false;
ConditionState last_conditions = {};
char last_phase[16] = "";

// ==================== FORWARD DECLARATIONS ====================
void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void apply_species_rules();
void publish_sensor_data();
void publish_output_state();
void publish_monitoring_state();
void set_output(const char* name, bool state);
void update_monitoring_state(bool log_changes);
void log_monitoring_changes();
void snapshot_monitoring_state();

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");

  strcpy(current_mode, DEFAULT_MODE);
  strcpy(current_species, "Rô Phi");

  outputs.pump = false;
  outputs.aerator = false;
  outputs.circulation = false;
  outputs.feeder = false;

  pinMode(PUMP_PIN, OUTPUT);
  pinMode(AERATOR_PIN, OUTPUT);
  pinMode(CIRCULATION_PIN, OUTPUT);
  pinMode(FEEDER_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(AERATOR_PIN, LOW);
  digitalWrite(CIRCULATION_PIN, LOW);
  digitalWrite(FEEDER_PIN, LOW);

#if SENSOR_TEST_MODE
  Serial.println("[INIT] SENSOR_TEST_MODE enabled - relays are locked OFF");
#endif

  Serial.println("[INIT] Initializing sensors...");

  waterTemp.begin();
  dht.begin();

  Wire.begin(21, 22);

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[OK] BH1750 Light Sensor initialized");
  } else {
    Serial.println("[WARN] BH1750 Light Sensor not found (optional)");
  }

  setup_wifi();

  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(512);

  update_monitoring_state(true);

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

  if (now - last_rule_engine >= RULE_ENGINE_INTERVAL && strcmp(current_mode, "AUTO") == 0 && !SENSOR_TEST_MODE) {
    apply_species_rules();
    last_rule_engine = now;
  }

  if (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL) {
    if (mqtt_client.connected()) {
      publish_sensor_data();
      publish_output_state();
      publish_monitoring_state();
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
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
    attempts++;
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

  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("[OK] MQTT Connected!");

    mqtt_client.publish(MQTT_TOPIC_STATUS, "online", true);

    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_PUMP);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_AERATOR);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_CIRCULATION);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_FEEDER);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_MODE);
    mqtt_client.subscribe(MQTT_TOPIC_CONFIG_SPECIES);

    publish_mqtt_discovery();

    mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
    mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
    mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
    mqtt_client.publish(MQTT_TOPIC_AERATOR, outputs.aerator ? "ON" : "OFF", true);
    mqtt_client.publish(MQTT_TOPIC_CIRCULATION, outputs.circulation ? "ON" : "OFF", true);
    mqtt_client.publish(MQTT_TOPIC_FEEDER, outputs.feeder ? "ON" : "OFF", true);
    publish_monitoring_state();

  } else {
    Serial.print("[WARN] MQTT connection failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

// ==================== HOME ASSISTANT MQTT DISCOVERY ====================
void publish_mqtt_discovery() {
  Serial.println("[HA Discovery] Publishing entity discoveries...");

  delay(100);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Water Temperature";
    doc["unique_id"] = "aquaculture_water_temp";
    doc["state_topic"] = MQTT_TOPIC_WATER_TEMP;
    doc["unit_of_measurement"] = "°C";
    doc["device_class"] = "temperature";
    doc["icon"] = "mdi:thermometer";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_water_temp/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture pH";
    doc["unique_id"] = "aquaculture_ph";
    doc["state_topic"] = MQTT_TOPIC_PH;
    doc["unit_of_measurement"] = "pH";
    doc["icon"] = "mdi:test-tube";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_ph/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Dissolved Oxygen";
    doc["unique_id"] = "aquaculture_do";
    doc["state_topic"] = MQTT_TOPIC_DO;
    doc["unit_of_measurement"] = "mg/L";
    doc["icon"] = "mdi:water";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_do/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture CO2";
    doc["unique_id"] = "aquaculture_co2";
    doc["state_topic"] = MQTT_TOPIC_CO2;
    doc["unit_of_measurement"] = "ppm";
    doc["icon"] = "mdi:molecule-co2";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_co2/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Turbidity";
    doc["unique_id"] = "aquaculture_turbidity";
    doc["state_topic"] = MQTT_TOPIC_TURBIDITY;
    doc["unit_of_measurement"] = "NTU";
    doc["icon"] = "mdi:water-opacity";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_turbidity/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Air Temperature";
    doc["unique_id"] = "aquaculture_air_temp";
    doc["state_topic"] = MQTT_TOPIC_AIR_TEMP;
    doc["unit_of_measurement"] = "°C";
    doc["device_class"] = "temperature";
    doc["icon"] = "mdi:thermometer";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_air_temp/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Humidity";
    doc["unique_id"] = "aquaculture_humidity";
    doc["state_topic"] = MQTT_TOPIC_HUMIDITY;
    doc["unit_of_measurement"] = "%";
    doc["device_class"] = "humidity";
    doc["icon"] = "mdi:water-percent";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_humidity/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Light Level";
    doc["unique_id"] = "aquaculture_light";
    doc["state_topic"] = MQTT_TOPIC_LIGHT;
    doc["unit_of_measurement"] = "lux";
    doc["icon"] = "mdi:lightbulb";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_light/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Pump";
    doc["unique_id"] = "aquaculture_pump";
    doc["state_topic"] = MQTT_TOPIC_PUMP;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_PUMP;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:pump";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/switch/aquaculture_pump/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Aerator";
    doc["unique_id"] = "aquaculture_aerator";
    doc["state_topic"] = MQTT_TOPIC_AERATOR;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_AERATOR;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:air-purifier";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/switch/aquaculture_aerator/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Circulation";
    doc["unique_id"] = "aquaculture_circulation";
    doc["state_topic"] = MQTT_TOPIC_CIRCULATION;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_CIRCULATION;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:water-pump";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/switch/aquaculture_circulation/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Feeder";
    doc["unique_id"] = "aquaculture_feeder";
    doc["state_topic"] = MQTT_TOPIC_FEEDER;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_FEEDER;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:fish-food";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/switch/aquaculture_feeder/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Mode";
    doc["unique_id"] = "aquaculture_mode";
    doc["state_topic"] = MQTT_TOPIC_MODE_STATE;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_MODE;
    doc["icon"] = "mdi:cog";
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

  delay(50);

  {
    StaticJsonDocument<768> doc;
    doc["name"] = "Aquaculture Species";
    doc["unique_id"] = "aquaculture_species";
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
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/select/aquaculture_species/config", payload.c_str(), true);
  }

  delay(50);

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Water Temp High";
    doc["unique_id"] = "aquaculture_condition_water_temp_high";
    doc["state_topic"] = MQTT_TOPIC_COND_TEMP_HIGH;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:thermometer-alert";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_water_temp_high/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Water Temp Low";
    doc["unique_id"] = "aquaculture_condition_water_temp_low";
    doc["state_topic"] = MQTT_TOPIC_COND_TEMP_LOW;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:snowflake-alert";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_water_temp_low/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture pH Low";
    doc["unique_id"] = "aquaculture_condition_ph_low";
    doc["state_topic"] = MQTT_TOPIC_COND_PH_LOW;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:flask-empty";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_ph_low/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture pH High";
    doc["unique_id"] = "aquaculture_condition_ph_high";
    doc["state_topic"] = MQTT_TOPIC_COND_PH_HIGH;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:flask";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_ph_high/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture DO Low";
    doc["unique_id"] = "aquaculture_condition_do_low";
    doc["state_topic"] = MQTT_TOPIC_COND_DO_LOW;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:water-alert";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_do_low/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture DO Critical";
    doc["unique_id"] = "aquaculture_condition_do_critical";
    doc["state_topic"] = MQTT_TOPIC_COND_DO_CRITICAL;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:alert-octagon";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_do_critical/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture CO2 High";
    doc["unique_id"] = "aquaculture_condition_co2_high";
    doc["state_topic"] = MQTT_TOPIC_COND_CO2_HIGH;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:molecule-co2";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_co2_high/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Turbidity High";
    doc["unique_id"] = "aquaculture_condition_turbidity_high";
    doc["state_topic"] = MQTT_TOPIC_COND_TURBIDITY_HIGH;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:water-opacity";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_turbidity_high/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Sensor Fault";
    doc["unique_id"] = "aquaculture_condition_sensor_fault";
    doc["state_topic"] = MQTT_TOPIC_COND_SENSOR_FAULT;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["device_class"] = "problem";
    doc["icon"] = "mdi:alert-circle";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_sensor_fault/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Any Active Alarm";
    doc["unique_id"] = "aquaculture_condition_any_active";
    doc["state_topic"] = MQTT_TOPIC_COND_ANY_ACTIVE;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:alarm-light";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_condition_any_active/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Outputs Locked";
    doc["unique_id"] = "aquaculture_outputs_locked";
    doc["state_topic"] = MQTT_TOPIC_COND_OUTPUTS_LOCKED;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:lock";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_outputs_locked/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Relay Test Status";
    doc["unique_id"] = "aquaculture_relay_test_status";
    doc["state_topic"] = MQTT_TOPIC_RELAY_TEST_STATUS;
    doc["icon"] = "mdi:power-plug-off";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_relay_test_status/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Production Phase";
    doc["unique_id"] = "aquaculture_production_phase";
    doc["state_topic"] = MQTT_TOPIC_PRODUCTION_PHASE;
    doc["icon"] = "mdi:factory";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_production_phase/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Production Ready";
    doc["unique_id"] = "aquaculture_production_ready";
    doc["state_topic"] = MQTT_TOPIC_PRODUCTION_READY;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["icon"] = "mdi:check-circle";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/binary_sensor/aquaculture_production_ready/config", payload.c_str(), true);
  }

  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Process Summary";
    doc["unique_id"] = "aquaculture_process_summary";
    doc["state_topic"] = MQTT_TOPIC_PROCESS_SUMMARY;
    doc["icon"] = "mdi:clipboard-text";
    doc["availability_topic"] = MQTT_TOPIC_STATUS;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/sensor/aquaculture_process_summary/config", payload.c_str(), true);
  }

  delay(100);

  Serial.println("[OK] All discoveries published!");
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

  if (strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0) {
    strcpy(current_mode, "MANUAL");
    set_output("pump", message == "ON");
    mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR) == 0) {
    set_output("aerator", message == "ON");
    mqtt_client.publish(MQTT_TOPIC_AERATOR, outputs.aerator ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_CIRCULATION) == 0) {
    set_output("circulation", message == "ON");
    mqtt_client.publish(MQTT_TOPIC_CIRCULATION, outputs.circulation ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_FEEDER) == 0) {
    set_output("feeder", message == "ON");
    mqtt_client.publish(MQTT_TOPIC_FEEDER, outputs.feeder ? "ON" : "OFF", true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_MODE) == 0) {
    strcpy(current_mode, message.c_str());
    Serial.print("[CONFIG] Mode changed to: ");
    Serial.println(current_mode);
    mqtt_client.publish(MQTT_TOPIC_MODE_STATE, current_mode, true);
  }

  if (strcmp(topic, MQTT_TOPIC_CONFIG_SPECIES) == 0) {
    strcpy(current_species, message.c_str());
    Serial.print("[CONFIG] Species changed to: ");
    Serial.println(current_species);
    mqtt_client.publish(MQTT_TOPIC_SPECIES_STATE, current_species, true);
  }

  update_monitoring_state(true);
  publish_monitoring_state();
}

// ==================== READ SENSORS ====================
void read_sensors() {
  waterTemp.requestTemperatures();
  float water_temp_reading = waterTemp.getTempCByIndex(0);
  sensors.water_temp = water_temp_reading;
  bool water_temp_disconnected = fabsf(water_temp_reading - DEVICE_DISCONNECTED_C) < 0.1f;
  sensor_validity.water_temp = !isnan(water_temp_reading) && !water_temp_disconnected && water_temp_reading > -20.0f && water_temp_reading < 60.0f;
  if (!sensor_validity.water_temp) sensors.water_temp = 0;

  sensors.air_temp = dht.readTemperature();
  sensors.air_humidity = dht.readHumidity();
  if (isnan(sensors.air_temp)) sensors.air_temp = 0;
  if (isnan(sensors.air_humidity)) sensors.air_humidity = 0;

  if (LIGHT_SENSOR_ENABLED) {
    sensors.light = lightMeter.readLightLevel();
    if (isnan(sensors.light) || sensors.light < 0) sensors.light = 0;
  } else {
    sensors.light = 0;
  }

  int ph_raw = analogRead(PH_PIN);
  int turbidity_raw = analogRead(TURBIDITY_PIN);
  int do_raw = analogRead(DO_PIN);

  sensors.ph = (ph_raw / 4095.0) * 14.0;
  sensors.turbidity = turbidity_raw;
  sensors.do_value = (do_raw / 4095.0) * 20.0;
  if (CO2_SENSOR_ENABLED) {
    sensors.co2 = (analogRead(CO2_PIN) / 4095.0) * 10.0;
  } else {
    sensors.co2 = 0;
  }

  sensor_validity.ph = ph_raw > 10 && ph_raw < 4085 && !isnan(sensors.ph) && sensors.ph >= 0.0f && sensors.ph <= 14.0f;
  sensor_validity.do_value = do_raw > 10 && do_raw < 4085 && !isnan(sensors.do_value) && sensors.do_value >= 0.0f && sensors.do_value <= 20.0f;
  sensor_validity.turbidity = turbidity_raw > 10 && turbidity_raw < 4085 && !isnan(sensors.turbidity) && sensors.turbidity >= 0.0f && sensors.turbidity <= 4095.0f;
  sensor_validity.co2 = !CO2_SENSOR_ENABLED || (!isnan(sensors.co2) && sensors.co2 >= 0.0f && sensors.co2 <= 100.0f);
  sensor_validity.required_ok = sensor_validity.water_temp && sensor_validity.ph && sensor_validity.do_value && sensor_validity.turbidity;

  sensors.last_read = millis();
  update_monitoring_state(true);

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

// ==================== SPECIES-BASED RULE ENGINE ====================
void apply_species_rules() {
  const SpeciesRule* rule = getSpeciesRule(current_species);

  bool pump_on = outputs.pump;
  bool aerator_on = outputs.aerator;
  bool circulation_on = outputs.circulation;
  bool feeder_on = outputs.feeder;

  bool alert_temp_high = conditions.water_temp_high;
  bool alert_temp_low = conditions.water_temp_low;
  bool alert_ph = conditions.ph_low || conditions.ph_high;
  bool alert_do_low = conditions.do_low;
  bool alert_do_critical = conditions.do_critical;
  bool alert_co2 = conditions.co2_high;
  bool alert_turbidity = conditions.turbidity_high;

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

  if (sensors.do_value < rule->do_min + 0.5) {
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

  if (water_stable) {
    feeder_on = false;
  } else {
    feeder_on = false;
  }

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
}

// ==================== SET OUTPUT ====================
void set_output(const char* name, bool state) {
  if (SENSOR_TEST_MODE && state) {
    Serial.print("[TEST] Suppressed ON command for ");
    Serial.println(name);
    state = false;
  }

  if (strcmp(name, "pump") == 0) {
    outputs.pump = state;
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
  } else if (strcmp(name, "aerator") == 0) {
    outputs.aerator = state;
  } else if (strcmp(name, "circulation") == 0) {
    outputs.circulation = state;
  } else if (strcmp(name, "feeder") == 0) {
    outputs.feeder = state;
  }

  Serial.print("[OUTPUT] ");
  Serial.print(name);
  Serial.println(state ? " ON" : " OFF");
}

// ==================== PUBLISH SENSOR DATA ====================
void publish_sensor_data() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP, String(sensors.water_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH, String(sensors.ph, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO, String(sensors.do_value, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_CO2, String(sensors.co2, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY, String(sensors.turbidity).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_AIR_TEMP, String(sensors.air_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY, String(sensors.air_humidity, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_LIGHT, String(sensors.light, 0).c_str(), true);
}

void update_monitoring_state(bool log_changes) {
  const SpeciesRule* rule = getSpeciesRule(current_species);
  if (rule == nullptr) {
    conditions.water_temp_high = false;
    conditions.water_temp_low = false;
    conditions.ph_low = false;
    conditions.ph_high = false;
    conditions.do_low = false;
    conditions.do_critical = false;
    conditions.co2_high = false;
    conditions.turbidity_high = false;
    conditions.sensor_fault = true;
    conditions.outputs_locked = SENSOR_TEST_MODE;
    conditions.any_active = true;
    strcpy(production_phase, "NOT_READY");
    production_ready = false;
    snprintf(process_summary, sizeof(process_summary), "species_profile_missing");
    if (log_changes) {
      log_monitoring_changes();
    }
    return;
  }

  conditions.water_temp_high = sensor_validity.water_temp && sensors.water_temp > rule->temp_max;
  conditions.water_temp_low = sensor_validity.water_temp && sensors.water_temp < rule->temp_min;
  conditions.ph_low = sensor_validity.ph && sensors.ph < rule->ph_min;
  conditions.ph_high = sensor_validity.ph && sensors.ph > rule->ph_max;
  conditions.do_low = sensor_validity.do_value && sensors.do_value < rule->do_min;
  conditions.do_critical = sensor_validity.do_value && sensors.do_value <= rule->do_critical;
  conditions.co2_high = CO2_SENSOR_ENABLED && sensor_validity.co2 && sensors.co2 > rule->co2_max;
  conditions.turbidity_high = sensor_validity.turbidity && sensors.turbidity > rule->turbidity_max;
  conditions.sensor_fault = !sensor_validity.required_ok || (CO2_SENSOR_ENABLED && !sensor_validity.co2);
  conditions.outputs_locked = SENSOR_TEST_MODE;
  conditions.any_active =
      conditions.water_temp_high ||
      conditions.water_temp_low ||
      conditions.ph_low ||
      conditions.ph_high ||
      conditions.do_low ||
      conditions.do_critical ||
      conditions.co2_high ||
      conditions.turbidity_high ||
      conditions.sensor_fault;

  bool blocking_required_alarm =
      conditions.water_temp_high ||
      conditions.water_temp_low ||
      conditions.ph_low ||
      conditions.ph_high ||
      conditions.do_low ||
      conditions.do_critical ||
      conditions.turbidity_high ||
      !sensor_validity.required_ok;

  if (SENSOR_TEST_MODE) {
    strcpy(production_phase, "SENSOR_TEST");
    production_ready = false;
    snprintf(process_summary, sizeof(process_summary), "required_valid=%s; relay_test=%s; outputs_locked=ON",
             sensor_validity.required_ok ? "YES" : "NO", relay_test_status);
  } else if (!sensor_validity.required_ok) {
    strcpy(production_phase, "VALIDATION");
    production_ready = false;
    snprintf(process_summary, sizeof(process_summary), "validation_failed: required sensor invalid");
  } else {
    production_ready = !blocking_required_alarm;
    if (production_ready) {
      strcpy(production_phase, "READY");
      snprintf(process_summary, sizeof(process_summary), "validation_ok; relay_test=%s", relay_test_status);
    } else {
      strcpy(production_phase, "NOT_READY");
      snprintf(process_summary, sizeof(process_summary), "validation_ok; active_condition_present");
    }
  }

  if (strcmp(current_mode, "SAFE") == 0 && !production_ready) {
    strncat(process_summary, "; mode=SAFE", sizeof(process_summary) - strlen(process_summary) - 1);
  }

  if (log_changes) {
    log_monitoring_changes();
  }
}

void log_monitoring_changes() {
  if (!monitor_state_initialized) {
    monitor_state_initialized = true;
    snapshot_monitoring_state();
    Serial.print("[PROCESS] Phase initialized: ");
    Serial.println(production_phase);
    Serial.print("[PROCESS] Ready: ");
    Serial.println(production_ready ? "ON" : "OFF");
    return;
  }

  if (strcmp(last_phase, production_phase) != 0) {
    Serial.print("[PROCESS] Phase changed: ");
    Serial.print(last_phase);
    Serial.print(" -> ");
    Serial.println(production_phase);
    strcpy(last_phase, production_phase);
  }

  if (last_production_ready != production_ready) {
    Serial.print("[PROCESS] Ready changed: ");
    Serial.print(last_production_ready ? "ON" : "OFF");
    Serial.print(" -> ");
    Serial.println(production_ready ? "ON" : "OFF");
    last_production_ready = production_ready;
  }

  if (last_conditions.any_active != conditions.any_active) {
    Serial.print("[CONDITION] any_active -> ");
    Serial.println(conditions.any_active ? "ON" : "OFF");
  }
  if (last_conditions.water_temp_high != conditions.water_temp_high) {
    Serial.print("[CONDITION] water_temp_high -> ");
    Serial.println(conditions.water_temp_high ? "ON" : "OFF");
  }
  if (last_conditions.water_temp_low != conditions.water_temp_low) {
    Serial.print("[CONDITION] water_temp_low -> ");
    Serial.println(conditions.water_temp_low ? "ON" : "OFF");
  }
  if (last_conditions.ph_low != conditions.ph_low) {
    Serial.print("[CONDITION] ph_low -> ");
    Serial.println(conditions.ph_low ? "ON" : "OFF");
  }
  if (last_conditions.ph_high != conditions.ph_high) {
    Serial.print("[CONDITION] ph_high -> ");
    Serial.println(conditions.ph_high ? "ON" : "OFF");
  }
  if (last_conditions.do_low != conditions.do_low) {
    Serial.print("[CONDITION] do_low -> ");
    Serial.println(conditions.do_low ? "ON" : "OFF");
  }
  if (last_conditions.do_critical != conditions.do_critical) {
    Serial.print("[CONDITION] do_critical -> ");
    Serial.println(conditions.do_critical ? "ON" : "OFF");
  }
  if (last_conditions.co2_high != conditions.co2_high) {
    Serial.print("[CONDITION] co2_high -> ");
    Serial.println(conditions.co2_high ? "ON" : "OFF");
  }
  if (last_conditions.turbidity_high != conditions.turbidity_high) {
    Serial.print("[CONDITION] turbidity_high -> ");
    Serial.println(conditions.turbidity_high ? "ON" : "OFF");
  }
  if (last_conditions.sensor_fault != conditions.sensor_fault) {
    Serial.print("[CONDITION] sensor_fault -> ");
    Serial.println(conditions.sensor_fault ? "ON" : "OFF");
  }
  if (last_conditions.outputs_locked != conditions.outputs_locked) {
    Serial.print("[CONDITION] outputs_locked -> ");
    Serial.println(conditions.outputs_locked ? "ON" : "OFF");
  }

  snapshot_monitoring_state();
}

void snapshot_monitoring_state() {
  last_conditions = conditions;
  strcpy(last_phase, production_phase);
  last_production_ready = production_ready;
}

void publish_monitoring_state() {
  if (!mqtt_client.connected()) {
    return;
  }

  mqtt_client.publish(MQTT_TOPIC_COND_TEMP_HIGH, conditions.water_temp_high ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_TEMP_LOW, conditions.water_temp_low ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_PH_LOW, conditions.ph_low ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_PH_HIGH, conditions.ph_high ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_DO_LOW, conditions.do_low ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_DO_CRITICAL, conditions.do_critical ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_CO2_HIGH, conditions.co2_high ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_TURBIDITY_HIGH, conditions.turbidity_high ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_SENSOR_FAULT, conditions.sensor_fault ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_ANY_ACTIVE, conditions.any_active ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_COND_OUTPUTS_LOCKED, conditions.outputs_locked ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_RELAY_TEST_STATUS, relay_test_status, true);
  mqtt_client.publish(MQTT_TOPIC_PRODUCTION_PHASE, production_phase, true);
  mqtt_client.publish(MQTT_TOPIC_PRODUCTION_READY, production_ready ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_PROCESS_SUMMARY, process_summary, true);
}

// ==================== PUBLISH OUTPUT STATE ====================
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