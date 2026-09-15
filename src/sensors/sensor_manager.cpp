#include <Wire.h>
#include "sensor_manager.h"
#include "../config/sensors_config.h"

SensorManager::SensorManager()
    : phase1Ads_(),
      temperatureDriver_(sensors_config::ONE_WIRE_PIN, sensors_config::ENABLE_DS18B20),
      phDriver_(phase1Ads_, sensors_config::PH_ADC_CHANNEL, sensors_config::ENABLE_PH),
      doDriver_(phase1Ads_, sensors_config::DO_ADC_CHANNEL, sensors_config::ENABLE_DO),
      waterLevelDriver_(sensors_config::WATER_LEVEL_PIN, sensors_config::ENABLE_WATER_LEVEL),
      ecDriver_(sensors_config::ENABLE_EC_TDS),
      orpDriver_(sensors_config::ENABLE_ORP),
      turbidityDriver_(sensors_config::ENABLE_TURBIDITY),
      co2Driver_(sensors_config::ENABLE_CO2),
      gasSensorDriver_(sensors_config::ENABLE_GAS_SENSOR) {}

void SensorManager::begin() {
  Wire.begin(sensors_config::I2C_SDA_PIN, sensors_config::I2C_SCL_PIN);
  const bool phase1AdsReady = phase1Ads_.begin(0x48);
  if (phase1AdsReady) {
    phase1Ads_.setGain(GAIN_TWOTHIRDS);
  }
  phDriver_.setInitialized(phase1AdsReady);
  doDriver_.setInitialized(phase1AdsReady);
  temperatureDriver_.begin();
  phDriver_.begin();
  doDriver_.begin();
  waterLevelDriver_.begin();
  ecDriver_.begin();
  orpDriver_.begin();
  turbidityDriver_.begin();
  co2Driver_.begin();
  gasSensorDriver_.begin();
}

void SensorManager::poll() {
  temperature_ = temperatureDriver_.read();
  ph_ = phDriver_.read();
  dissolvedOxygen_ = doDriver_.read();
  waterLevel_ = waterLevelDriver_.read();
  ec_ = ecDriver_.read();
  orp_ = orpDriver_.read();
  turbidity_ = turbidityDriver_.read();
  co2_ = co2Driver_.read();
  gasSensor_ = gasSensorDriver_.read();
}
