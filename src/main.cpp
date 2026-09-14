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

// ==================== GLOBAL VARIABLES ====================
SensorData sensors;
OutputState outputs;

uint32_t last_sensor_read = 0;
uint32_t last_mqtt_publish = 0;
uint32_t last_rule_engine = 0;

char current_mode[16] = "";
char current_species[32] = "Tilapia";

// ==================== FORWARD DECLARATIONS ====================
void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_discovery();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void read_sensors();
void apply_rules();
void publish_sensor_data();
void publish_output_state();
void set_output(const char* name, bool state);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n===== SmartFarm Aquaculture Controller V" FW_VERSION " =====");
  Serial.println("Starting initialization...");
  
  // Set default mode
  strcpy(current_mode, DEFAULT_MODE);
  
  // Initialize pins
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  
  // Initialize sensors
  Serial.println("[INIT] Initializing sensors...");
  
  waterTemp.begin();
  dht.begin();
  
  Wire.begin(21, 22);
  
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[OK] BH1750 Light Sensor initialized");
  } else {
    Serial.println("[WARN] BH1750 Light Sensor not found (optional)");
  }
  
  // Initialize WiFi
  setup_wifi();
  
  // Initialize MQTT
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_client.setBufferSize(512);
  
  Serial.println("===== Initialization Complete =====\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  // Maintain connections
  if (!mqtt_client.connected()) {
    reconnect_mqtt();
  } else {
    mqtt_client.loop();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  
  uint32_t now = millis();
  
  // Read sensors
  if (now - last_sensor_read >= SENSOR_READ_INTERVAL) {
    read_sensors();
    last_sensor_read = now;
  }
  
  // Run rule engine
  if (now - last_rule_engine >= RULE_ENGINE_INTERVAL && strcmp(current_mode, "AUTO") == 0) {
    apply_rules();
    last_rule_engine = now;
  }
  
  // Publish MQTT
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
  if (mqtt_client.connected()) {
    return;
  }
  
  static uint32_t last_reconnect_attempt = 0;
  uint32_t now = millis();
  
  // Only attempt reconnect every 5 seconds
  if (now - last_reconnect_attempt < MQTT_RECONNECT_INTERVAL) {
    return;
  }
  last_reconnect_attempt = now;
  
  Serial.print("[MQTT] Connecting to: ");
  Serial.println(MQTT_BROKER);
  
  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("[OK] MQTT Connected!");
    
    // Publish online status
    mqtt_client.publish("homeassistant/switch/aquaculture_status/state", "online");
    
    // Subscribe to control topics
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_PUMP);
    mqtt_client.subscribe(MQTT_TOPIC_CONTROL_MODE);
    mqtt_client.subscribe(MQTT_TOPIC_CONFIG_SPECIES);
    
    // Publish Home Assistant Discovery
    publish_mqtt_discovery();
    
  } else {
    Serial.print("[WARN] MQTT connection failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

// ==================== HOME ASSISTANT MQTT DISCOVERY ====================
void publish_mqtt_discovery() {
  Serial.println("[HA Discovery] Publishing entity discoveries...");
  
  delay(100); // Give broker time between publishes
  
  // Temperature Sensor
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
  
  // pH Sensor
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
  
  // Dissolved Oxygen
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
  
  // CO2 Sensor
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
  
  // Turbidity Sensor
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
  
  // Air Temperature
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
  
  // Humidity
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
  
  // Light Level
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
  
  // Pump Switch
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
  
  // Mode Selector
  {
    StaticJsonDocument<512> doc;
    doc["name"] = "Aquaculture Mode";
    doc["unique_id"] = "aquaculture_mode";
    doc["state_topic"] = MQTT_TOPIC_CONTROL_MODE;
    doc["command_topic"] = MQTT_TOPIC_CONTROL_MODE;
    doc["icon"] = "mdi:cog";
    doc["options"][0] = "AUTO";
    doc["options"][1] = "MANUAL";
    doc["device"]["identifiers"][0] = MQTT_CLIENT_ID;
    doc["device"]["name"] = "Aquaculture Controller";
    
    String payload;
    serializeJson(doc, payload);
    mqtt_client.publish("homeassistant/select/aquaculture_mode/config", payload.c_str(), true);
  }
  
  delay(50);
  
  // Species Selector
  {
    StaticJsonDocument<768> doc;
    doc["name"] = "Aquaculture Species";
    doc["unique_id"] = "aquaculture_species";
    doc["state_topic"] = MQTT_TOPIC_CONFIG_SPECIES;
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
  
  // Control pump
  if (strcmp(topic, MQTT_TOPIC_CONTROL_PUMP) == 0) {
    strcpy(current_mode, "MANUAL");
    set_output("pump", message == "ON");
  }
  
  // Control mode
  if (strcmp(topic, MQTT_TOPIC_CONTROL_MODE) == 0) {
    strcpy(current_mode, message.c_str());
    Serial.print("[CONFIG] Mode changed to: ");
    Serial.println(current_mode);
  }
  
  // Change species
  if (strcmp(topic, MQTT_TOPIC_CONFIG_SPECIES) == 0) {
    strcpy(current_species, message.c_str());
    Serial.print("[CONFIG] Species changed to: ");
    Serial.println(current_species);
  }
}

// ==================== READ SENSORS ====================
void read_sensors() {
  // Water temperature
  waterTemp.requestTemperatures();
  sensors.water_temp = waterTemp.getTempCByIndex(0);
  if (sensors.water_temp == -127) sensors.water_temp = 0;
  
  // Air temperature & humidity
  sensors.air_temp = dht.readTemperature();
  sensors.air_humidity = dht.readHumidity();
  if (isnan(sensors.air_temp)) sensors.air_temp = 0;
  if (isnan(sensors.air_humidity)) sensors.air_humidity = 0;
  
  // Light
  sensors.light = lightMeter.readLightLevel();
  if (sensors.light < 0) sensors.light = 0;
  
  // Analog sensors
  sensors.ph = (analogRead(PH_PIN) / 4095.0) * 14.0;
  sensors.turbidity = analogRead(TURBIDITY_PIN);
  sensors.do_value = (analogRead(DO_PIN) / 4095.0) * 20.0;
  sensors.co2 = (analogRead(CO2_PIN) / 4095.0) * 10.0;
  
  sensors.last_read = millis();
  
  // Debug output
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
  Serial.print(sensors.air_humidity);
  Serial.println("%");
  Serial.print("Light: ");
  Serial.print(sensors.light);
  Serial.println(" lux");
}

// ==================== APPLY RULES ====================
void apply_rules() {
  bool pump_on = false;
  bool aerator_on = false;
  bool circulation_on = false;
  
  // Rule: High temperature -> enable pump/circulation
  if (sensors.water_temp > AUTO_PUMP_TEMP_HIGH) {
    pump_on = true;
    circulation_on = true;
  }
  
  // Rule: Low dissolved oxygen -> enable aerator and pump
  if (sensors.do_value < AUTO_PUMP_DO_LOW) {
    pump_on = true;
    aerator_on = true;
  }
  
  // Rule: Very low DO -> critical
  if (sensors.do_value < DO_CRITICAL_LOW) {
    aerator_on = true;
    pump_on = true;
  }
  
  // Rule: High CO2 -> enable pump
  if (sensors.co2 > AUTO_PUMP_CO2_HIGH) {
    pump_on = true;
  }
  
  // Apply outputs
  set_output("pump", pump_on);
  set_output("aerator", aerator_on);
  set_output("circulation", circulation_on);
  
  if (pump_on || aerator_on || circulation_on) {
    Serial.println("[RULES] Auto control activated");
  }
}

// ==================== SET OUTPUT ====================
void set_output(const char* name, bool state) {
  if (strcmp(name, "pump") == 0) {
    outputs.pump = state;
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
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
  
  // Publish individual sensor values
  mqtt_client.publish(MQTT_TOPIC_WATER_TEMP, String(sensors.water_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_PH, String(sensors.ph, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_DO, String(sensors.do_value, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_CO2, String(sensors.co2, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_TURBIDITY, String(sensors.turbidity).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_AIR_TEMP, String(sensors.air_temp, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_HUMIDITY, String(sensors.air_humidity, 2).c_str(), true);
  mqtt_client.publish(MQTT_TOPIC_LIGHT, String(sensors.light, 0).c_str(), true);
}

// ==================== PUBLISH OUTPUT STATE ====================
void publish_output_state() {
  if (!mqtt_client.connected()) {
    return;
  }
  
  mqtt_client.publish(MQTT_TOPIC_PUMP, outputs.pump ? "ON" : "OFF", true);
  mqtt_client.publish(MQTT_TOPIC_CONTROL_MODE, current_mode, true);
  mqtt_client.publish(MQTT_TOPIC_CONFIG_SPECIES, current_species, true);
}
