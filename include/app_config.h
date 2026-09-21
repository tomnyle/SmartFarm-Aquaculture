#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// Firmware Version
#define FW_VERSION "2.0.0"
#define FW_BUILD_DATE __DATE__
#define FW_DEVICE_ID "ESP32_AQUACULTURE_V2"
#define FW_HW_VERSION "ESP32DEV"

// ==================== WIFI CONFIGURATION ====================
#define WIFI_SSID "Le Danh"
#define WIFI_PASSWORD "123456789"
#define WIFI_CONNECT_TIMEOUT 30000  // 30 seconds

// ==================== MQTT CONFIGURATION ====================
#define MQTT_BROKER "192.168.100.168"
#define MQTT_PORT 1883
#define MQTT_USER "homer"
#define MQTT_PASSWORD "Danh@@@1992"
#define MQTT_CLIENT_ID "ESP32_AQUACULTURE"
#define MQTT_RECONNECT_INTERVAL 5000

// ==================== HOME ASSISTANT MQTT DISCOVERY ====================
#define HA_DISCOVERY_PREFIX "homeassistant"
#define HA_DISCOVERY_ENABLED true

// ==================== MQTT TOPICS (APPLICATION NAMESPACE) ====================
// Sensor Topics - State
#define MQTT_TOPIC_WATER_TEMP "smartfarm/aquaculture/sensor/water_temp"
#define MQTT_TOPIC_PH "smartfarm/aquaculture/sensor/ph"
#define MQTT_TOPIC_PH_TREND "smartfarm/aquaculture/sensor/ph_trend"
#define MQTT_TOPIC_TURBIDITY "smartfarm/aquaculture/sensor/turbidity"
#define MQTT_TOPIC_DO "smartfarm/aquaculture/sensor/do"
#define MQTT_TOPIC_CO2 "smartfarm/aquaculture/sensor/co2"
#define MQTT_TOPIC_AIR_TEMP "smartfarm/aquaculture/sensor/air_temp"
#define MQTT_TOPIC_HUMIDITY "smartfarm/aquaculture/sensor/humidity"
#define MQTT_TOPIC_LIGHT "smartfarm/aquaculture/sensor/light"
#define MQTT_TOPIC_WATER_LEVEL "smartfarm/aquaculture/sensor/water_level"
#define MQTT_TOPIC_AERATOR_CURRENT "smartfarm/aquaculture/sensor/aerator_current"
#define MQTT_TOPIC_PUMP_CURRENT "smartfarm/aquaculture/sensor/pump_current"

// Output Topics - State
#define MQTT_TOPIC_PUMP "smartfarm/aquaculture/output/pump"
#define MQTT_TOPIC_AERATOR "smartfarm/aquaculture/output/aerator"
#define MQTT_TOPIC_AERATOR_1 "smartfarm/aquaculture/output/aerator_1"
#define MQTT_TOPIC_AERATOR_2 "smartfarm/aquaculture/output/aerator_2"
#define MQTT_TOPIC_CIRCULATION "smartfarm/aquaculture/output/circulation"
#define MQTT_TOPIC_FEEDER "smartfarm/aquaculture/output/feeder"
#define MQTT_TOPIC_ALARM_OUTPUT "smartfarm/aquaculture/output/alarm"

// Control Topics - Command
#define MQTT_TOPIC_CONTROL_PUMP "smartfarm/aquaculture/control/pump/set"
#define MQTT_TOPIC_CONTROL_AERATOR "smartfarm/aquaculture/control/aerator/set"
#define MQTT_TOPIC_CONTROL_AERATOR_1 "smartfarm/aquaculture/control/aerator_1/set"
#define MQTT_TOPIC_CONTROL_AERATOR_2 "smartfarm/aquaculture/control/aerator_2/set"
#define MQTT_TOPIC_CONTROL_CIRCULATION "smartfarm/aquaculture/control/circulation/set"
#define MQTT_TOPIC_CONTROL_FEEDER "smartfarm/aquaculture/control/feeder/set"
#define MQTT_TOPIC_CONTROL_ALARM_OUTPUT "smartfarm/aquaculture/control/alarm/set"
#define MQTT_TOPIC_CONTROL_MODE "smartfarm/aquaculture/config/mode/set"
#define MQTT_TOPIC_CONFIG_SPECIES "smartfarm/aquaculture/config/species/set"

// Select state topics
#define MQTT_TOPIC_MODE_STATE "smartfarm/aquaculture/config/mode/state"
#define MQTT_TOPIC_SPECIES_STATE "smartfarm/aquaculture/config/species/state"

// Status Topic
#define MQTT_TOPIC_STATUS "smartfarm/aquaculture/status"
#define MQTT_TOPIC_STATE "smartfarm/aquaculture/controller/state"
#define MQTT_TOPIC_SAFETY_STATE "smartfarm/aquaculture/safety/state"
#define MQTT_TOPIC_ALARM_TEXT "smartfarm/aquaculture/alarm/text"
#define MQTT_TOPIC_ALARM_ACTIVE "smartfarm/aquaculture/alarm/active"
#define MQTT_TOPIC_SAFETY_ACTIVE "smartfarm/aquaculture/safety/active"
#define MQTT_TOPIC_EMERGENCY_ACTIVE "smartfarm/aquaculture/emergency/active"
#define MQTT_TOPIC_CONTROLLER_STATUS "smartfarm/aquaculture/controller/status"
#define MQTT_TOPIC_TEST_MODE "smartfarm/aquaculture/controller/test_mode"

// Availability Topics
#define MQTT_TOPIC_AVAILABILITY "smartfarm/aquaculture/availability"
#define MQTT_TOPIC_AVAILABILITY_CO2 "smartfarm/aquaculture/availability/co2"
#define MQTT_TOPIC_AVAILABILITY_LIGHT "smartfarm/aquaculture/availability/light"
#define MQTT_TOPIC_AVAILABILITY_WATER_LEVEL "smartfarm/aquaculture/availability/water_level"
#define MQTT_TOPIC_AVAILABILITY_AERATOR_CURRENT "smartfarm/aquaculture/availability/aerator_current"
#define MQTT_TOPIC_AVAILABILITY_PUMP_CURRENT "smartfarm/aquaculture/availability/pump_current"
#define MQTT_TOPIC_AVAILABILITY_AERATOR_2 "smartfarm/aquaculture/availability/aerator_2"
#define MQTT_TOPIC_AVAILABILITY_ALARM_OUTPUT "smartfarm/aquaculture/availability/alarm_output"

// ==================== DEVICE CONFIGURATION ====================
#define DEVICE_NAME "Aquaculture-Controller-001"
#define DEVICE_LOCATION "Home Pond"

// ==================== HOME ASSISTANT DEVICE METADATA ====================
#define DEVICE_DISPLAY_NAME "Aquaculture Controller V2"
#define DEVICE_MANUFACTURER "SmartFarm"
#define DEVICE_MODEL "ESP32 Aquaculture Controller"
#define DEVICE_CONFIG_URL "http://192.168.100.168:8123"

// ==================== SENSOR READ INTERVALS ====================
#define SENSOR_READ_INTERVAL 5000      // 5 seconds
#define MQTT_PUBLISH_INTERVAL 10000    // 10 seconds
#define RULE_ENGINE_INTERVAL 5000      // 5 seconds

// ==================== OTA UPDATE ====================
#define OTA_ENABLED false
#define OTA_PORT 3232

// ==================== WATCHDOG TIMER ====================
#define WATCHDOG_ENABLED true
#define WATCHDOG_TIMEOUT 30000  // 30 seconds

// ==================== LOGGING ====================
#define LOGGING_ENABLED true
#define LOG_LEVEL 3  // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG

// ==================== DEFAULT MODE ====================
#define DEFAULT_MODE "AUTO"  // AUTO, MANUAL, SCHEDULE, SAFE

// ==================== SENSOR/OUTPUT FEATURE FLAGS ====================
#define CO2_SENSOR_ENABLED false
#define WATER_LEVEL_SENSOR_ENABLED false
#define AERATOR_CURRENT_SENSOR_ENABLED false
#define PUMP_CURRENT_SENSOR_ENABLED false
#define AERATOR_2_HARDWARE_AVAILABLE false
#define ALARM_OUTPUT_HARDWARE_AVAILABLE false

// ==================== TEST MODES ====================
#define BENCH_TEST_MODE false
#define SENSOR_TEST_MODE false

#if BENCH_TEST_MODE && SENSOR_TEST_MODE
#error "BENCH_TEST_MODE and SENSOR_TEST_MODE cannot both be true"
#endif

// ==================== BENCH MODE ====================
#define BENCH_DEFAULT_WATER_TEMP 27.0f
#define BENCH_DEFAULT_PH 7.20f
#define BENCH_DEFAULT_PH_TREND 0.00f
#define BENCH_DEFAULT_DO 6.50f
#define BENCH_DEFAULT_CO2 0.00f
#define BENCH_DEFAULT_TURBIDITY 0.00f
#define BENCH_DEFAULT_AIR_TEMP 28.0f
#define BENCH_DEFAULT_AIR_HUMIDITY 65.0f
#define BENCH_DEFAULT_LIGHT 0.0f
#define BENCH_DEFAULT_WATER_LEVEL 70.0f
#define BENCH_DEFAULT_AERATOR_CURRENT 0.00f
#define BENCH_DEFAULT_PUMP_CURRENT 0.00f

// ==================== ADC + ANALOG SENSOR CALIBRATION ====================
#define ANALOG_READ_SAMPLES 16
#define ANALOG_SAMPLE_DELAY_US 250
#define ADC_REFERENCE_VOLTAGE 3.30f

// pH mapping: pH = 7 + (PH_NEUTRAL_VOLTAGE - voltage) / PH_SLOPE_VOLT_PER_PH
#define PH_NEUTRAL_VOLTAGE 2.50f
#define PH_SLOPE_VOLT_PER_PH 0.18f
#define PH_MIN_VALUE 0.0f
#define PH_MAX_VALUE 14.0f

// DO mapping: linear interpolation between 0mg/L and full-scale mg/L
#define DO_ZERO_VOLTAGE 0.40f
#define DO_FULL_SCALE_VOLTAGE 2.20f
#define DO_FULL_SCALE_MG_L 12.0f
#define DO_MAX_VALUE 20.0f

// CO2 mapping (optional analog input)
#define CO2_ZERO_VOLTAGE 0.40f
#define CO2_FULL_SCALE_VOLTAGE 2.00f
#define CO2_FULL_SCALE_PPM 10.0f
#define CO2_MAX_VALUE 100.0f

// Turbidity: only publish calibrated NTU when this flag is true
#define TURBIDITY_CALIBRATED false
#define TURBIDITY_ZERO_NTU_VOLTAGE 2.50f
#define TURBIDITY_MAX_NTU_VOLTAGE 0.50f
#define TURBIDITY_MAX_NTU 3000.0f

// Optional analog sensors (enabled only when *_SENSOR_ENABLED=true and pin mapped to ADC1)
#define WATER_LEVEL_ADC_EMPTY 800
#define WATER_LEVEL_ADC_FULL 3200
#define WATER_LEVEL_PERCENT_EMPTY 0.0f
#define WATER_LEVEL_PERCENT_FULL 100.0f

#define AERATOR_CURRENT_ZERO_VOLTAGE 0.50f
#define AERATOR_CURRENT_AMP_PER_VOLT 10.0f
#define AERATOR_CURRENT_MAX_VALUE 100.0f

#define PUMP_CURRENT_ZERO_VOLTAGE 0.50f
#define PUMP_CURRENT_AMP_PER_VOLT 10.0f
#define PUMP_CURRENT_MAX_VALUE 100.0f

// ==================== CONTROL THRESHOLDS ====================
// Temperature (°C)
#define TEMP_ALERT_HIGH 30
#define TEMP_ALERT_LOW 15

// pH
#define PH_ALERT_HIGH 8.0
#define PH_ALERT_LOW 6.5

// Dissolved Oxygen (mg/L)
#define DO_ALERT_LOW 5.0
#define DO_CRITICAL_LOW 3.0

// CO2 (ppm)
#define CO2_ALERT_HIGH 5.0

// ==================== OUTPUT CONTROL ====================
// Auto control conditions
#define AUTO_PUMP_TEMP_HIGH 30
#define AUTO_PUMP_DO_LOW 5.0
#define AUTO_PUMP_CO2_HIGH 5.0
#define AUTO_AERATOR_DO_LOW 4.0
#define AUTO_CIRCULATION_TEMP_HIGH 28

// Pump minimum on time
#define PUMP_MIN_ON_TIME 10000
#define PUMP_MIN_OFF_TIME 30000

#endif // APP_CONFIG_H