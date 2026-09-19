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
bool bh1750_ready = false;

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
float ph_trend_per_hour = 0.0f;
float last_ph_sample = NAN;
uint32_t last_ph_sample_ms = 0;

char current_mode[16] = "";
char current_species[32] = "Rô Phi";
char mqtt_client_id[64] = "";

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
void initialize_outputs();
void build_mqtt_client_id();
void update_ph_trend(float current_ph, uint32_t sample_time_ms);
bool mqtt_connect_with_auth(bool use_lwt);
bool mqtt_connect_without_auth(bool use_lwt);
bool publish_float_topic(const char* topic, float value, uint8_t decimals);
const char* mqtt_device_identifier();

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

  initialize_outputs();

  Serial.println("[INIT] Initializing sensors...");
  if (BENCH_TEST_MODE) {
    Serial.println("[INIT] BENCH_TEST_MODE enabled - relay outputs stay OFF and analog sensors use safe defaults");
  }

  waterTemp.begin();
  dht.begin();

  Wire.begin(I2C_SDA, I2C_SCL);

  bh1750_ready = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (bh1750_ready) {
    Serial.println("[OK] BH1750 Light Sensor initialized");
  } else {
    Serial.println("[WARN] BH1750 Light Sensor not found (optional)");
  }

  setup_wifi();

  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(512);

  Serial.println("===== Initialization Complete =====\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  } else if (!mqtt_client.connected()) {
    reconnect_mqtt();
  } else {
    mqtt_client.loop();
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
  if (mqtt_client.connected() || WiFi.status() != WL_CONNECTED) {
    return;
  }

  static uint32_t last_reconnect_attempt = 0;
  uint32_t now = millis();

  if (now - last_reconnect_attempt < MQTT_RECONNECT_INTERVAL) {
    return;
  }
  last_reconnect_attempt = now;

  build_mqtt_client_id();

  Serial.print("[MQTT] Connecting to ");
  Serial.print(MQTT_BROKER);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" as ");
  Serial.print(mqtt_client_id);
  Serial.print(" (user=");
  Serial.print((strlen(MQTT_USER) > 0) ? MQTT_USER : "none");
  Serial.print(", lwt=");
  Serial.print(MQTT_ENABLE_LWT ? "on" : "off");
  Serial.println(")");

  bool connected = false;
  if (strlen(MQTT_USER) > 0) {
    connected = mqtt_connect_with_auth(MQTT_ENABLE_LWT);
  } else {
    connected = mqtt_connect_without_auth(MQTT_ENABLE_LWT);
  }

  if (!connected && MQTT_ENABLE_LWT && mqtt_client.state() == 5) {
    Serial.println("[MQTT] Broker rejected LWT/auth combination (rc=5), retrying without LWT");
    connected = (strlen(MQTT_USER) > 0) ? mqtt_connect_with_auth(false) : mqtt_connect_without_auth(false);
  }

  if (connected) {
    Serial.println("[OK] MQTT Connected!");

    mqtt_client.publish(MQTT_TOPIC_STATUS, MQTT_LWT_PAYLOAD_ONLINE, true);

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

  } else {
    Serial.print("[WARN] MQTT connection failed, state=");
    Serial.print(mqtt_client.state());
    Serial.print(", WiFi=");
    Serial.println(WiFi.status());
  }
}

void initialize_outputs() {
  const uint8_t output_pins[] = {PUMP_PIN, AERATOR_PIN, CIRCULATION_PIN, FEEDER_PIN};
  for (uint8_t i = 0; i < sizeof(output_pins) / sizeof(output_pins[0]); i++) {
    pinMode(output_pins[i], OUTPUT);
    digitalWrite(output_pins[i], LOW);
  }
}

void build_mqtt_client_id() {
  if (mqtt_client_id[0] != '\0') {
    return;
  }

  String mac = WiFi.macAddress();
  mac.replace(":", "");
  snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s", MQTT_CLIENT_ID, mac.c_str());
}

bool mqtt_connect_with_auth(bool use_lwt) {
  if (use_lwt) {
    return mqtt_client.connect(
      mqtt_client_id,
      MQTT_USER,
      MQTT_PASSWORD,
      MQTT_LWT_TOPIC,
      1,
      true,
      MQTT_LWT_PAYLOAD_OFFLINE
    );
  }

  return mqtt_client.connect(mqtt_client_id, MQTT_USER, MQTT_PASSWORD);
}

bool mqtt_connect_without_auth(bool use_lwt) {
  if (use_lwt) {
    return mqtt_client.connect(
      mqtt_client_id,
      MQTT_LWT_TOPIC,
      1,
      true,
      MQTT_LWT_PAYLOAD_OFFLINE
    );
  }

  return mqtt_client.connect(mqtt_client_id);
}

const char* mqtt_device_identifier() {
  return mqtt_client_id[0] != '\0' ? mqtt_client_id : MQTT_CLIENT_ID;
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
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
    doc["device"]["identifiers"][0] = mqtt_device_identifier();
    doc["device"]["name"] = "Aquaculture Controller";

    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/select/aquaculture_species/config", payload.c_str(), true);
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
    publish_output_state();
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_AERATOR) == 0) {
    set_output("aerator", message == "ON");
    publish_output_state();
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_CIRCULATION) == 0) {
    set_output("circulation", message == "ON");
    publish_output_state();
  }

  if (strcmp(topic, MQTT_TOPIC_CONTROL_FEEDER) == 0) {
    set_output("feeder", message == "ON");
    publish_output_state();
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
}

// ==================== READ SENSORS ====================
void read_sensors() {
  waterTemp.requestTemperatures();
  sensors.water_temp = waterTemp.getTempCByIndex(0);
  if (sensors.water_temp == -127) {
    sensors.water_temp = BENCH_TEST_MODE ? BENCH_DEFAULT_WATER_TEMP : 0;
  }

  sensors.air_temp = dht.readTemperature();
  sensors.air_humidity = dht.readHumidity();
  if (isnan(sensors.air_temp)) sensors.air_temp = BENCH_TEST_MODE ? BENCH_DEFAULT_AIR_TEMP : 0;
  if (isnan(sensors.air_humidity)) sensors.air_humidity = BENCH_TEST_MODE ? BENCH_DEFAULT_AIR_HUMIDITY : 0;

  sensors.light = bh1750_ready ? lightMeter.readLightLevel() : BENCH_DEFAULT_LIGHT;
  if (sensors.light < 0) sensors.light = 0;

  sensors.last_read = millis();
  if (BENCH_TEST_MODE) {
    sensors.ph = BENCH_DEFAULT_PH;
    sensors.turbidity = BENCH_DEFAULT_TURBIDITY;
    sensors.do_value = BENCH_DEFAULT_DO;
    sensors.co2 = 0.0f;
  } else {
    sensors.ph = (static_cast<float>(analogRead(PH_PIN)) / 4095.0f) * 14.0f;
    sensors.turbidity = static_cast<float>(analogRead(TURBIDITY_PIN));
    sensors.do_value = (static_cast<float>(analogRead(DO_PIN)) / 4095.0f) * 20.0f;
    sensors.co2 = CO2_SENSOR_ENABLED ? (static_cast<float>(analogRead(CO2_PIN)) / 4095.0f) * 10.0f : 0.0f;
  }
  update_ph_trend(sensors.ph, sensors.last_read);

  Serial.println("===== SENSOR READINGS =====");
  Serial.print("Water Temp: ");
  Serial.print(sensors.water_temp);
  Serial.println("°C");
  Serial.print("pH: ");
  Serial.println(sensors.ph);
  Serial.print("pH Trend: ");
  Serial.print(ph_trend_per_hour);
  Serial.println(" pH/h");
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
  if (BENCH_TEST_MODE) {
    static bool bench_logged = false;
    if (!bench_logged) {
      Serial.println("[RULES] BENCH_TEST_MODE active - automatic relay control is suppressed");
      bench_logged = true;
    }
    return;
  }

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
  bool effective_state = (BENCH_TEST_MODE && state) ? false : state;
  if (BENCH_TEST_MODE && state) {
    Serial.print("[BENCH] Suppressing ON request for ");
    Serial.println(name);
  }

  if (strcmp(name, "pump") == 0) {
    outputs.pump = effective_state;
    digitalWrite(PUMP_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "aerator") == 0) {
    outputs.aerator = effective_state;
    digitalWrite(AERATOR_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "circulation") == 0) {
    outputs.circulation = effective_state;
    digitalWrite(CIRCULATION_PIN, effective_state ? HIGH : LOW);
  } else if (strcmp(name, "feeder") == 0) {
    outputs.feeder = effective_state;
    digitalWrite(FEEDER_PIN, effective_state ? HIGH : LOW);
  }

  Serial.print("[OUTPUT] ");
  Serial.print(name);
  Serial.println(effective_state ? " ON" : " OFF");
}

// ==================== PUBLISH SENSOR DATA ====================
void publish_sensor_data() {
  if (!mqtt_client.connected()) {
    return;
  }

  publish_float_topic(MQTT_TOPIC_WATER_TEMP, sensors.water_temp, 2);
  publish_float_topic(MQTT_TOPIC_PH, sensors.ph, 2);
  publish_float_topic(MQTT_TOPIC_DO, sensors.do_value, 2);
  publish_float_topic(MQTT_TOPIC_CO2, sensors.co2, 2);
  publish_float_topic(MQTT_TOPIC_TURBIDITY, sensors.turbidity, 0);
  publish_float_topic(MQTT_TOPIC_AIR_TEMP, sensors.air_temp, 2);
  publish_float_topic(MQTT_TOPIC_HUMIDITY, sensors.air_humidity, 2);
  publish_float_topic(MQTT_TOPIC_LIGHT, sensors.light, 0);
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

void update_ph_trend(float current_ph, uint32_t sample_time_ms) {
  if (current_ph <= 0.0f || isnan(current_ph)) {
    return;
  }

  if (last_ph_sample_ms != 0 && sample_time_ms > last_ph_sample_ms) {
    uint32_t elapsed = sample_time_ms - last_ph_sample_ms;
    if (elapsed >= PH_TREND_MIN_INTERVAL_MS) {
      ph_trend_per_hour = ((current_ph - last_ph_sample) * 3600000.0f) / static_cast<float>(elapsed);
      last_ph_sample = current_ph;
      last_ph_sample_ms = sample_time_ms;
    }
    return;
  }

  last_ph_sample = current_ph;
  last_ph_sample_ms = sample_time_ms;
  ph_trend_per_hour = 0.0f;
}

bool publish_float_topic(const char* topic, float value, uint8_t decimals) {
  char payload[24];
  snprintf(payload, sizeof(payload), "%.*f", decimals, static_cast<double>(value));
  return mqtt_client.publish(topic, payload, true);
}