#ifndef SRC_SENSORS_SENSOR_MANAGER_H
#define SRC_SENSORS_SENSOR_MANAGER_H

#include "temperature/ds18b20_driver.h"
#include "ph/ph_sensor_driver.h"
#include "dissolved_oxygen/do_sensor_driver.h"
#include "water_level/water_level_driver.h"
#include "ec_tds/ec_tds_driver.h"
#include "orp/orp_driver.h"
#include "turbidity/turbidity_driver.h"
#include "co2/co2_driver.h"
#include "gas_sensor/gas_sensor_driver.h"
#include "../utils/data_types.h"

class SensorManager {
 public:
  SensorManager();
  void begin();
  void poll();
  SensorReading getTemperature() const { return temperature_; }
  SensorReading getPH() const { return ph_; }
  SensorReading getDO() const { return dissolvedOxygen_; }
  SensorReading getWaterLevel() const { return waterLevel_; }
  SensorReading getEC() const { return ec_; }
  SensorReading getORP() const { return orp_; }
  SensorReading getTurbidity() const { return turbidity_; }
  SensorReading getCO2() const { return co2_; }
  SensorReading getGasSensor() const { return gasSensor_; }

 private:
  DS18B20Driver temperatureDriver_;
  PHSensorDriver phDriver_;
  DOSensorDriver doDriver_;
  WaterLevelDriver waterLevelDriver_;
  ECTDSDriver ecDriver_;
  ORPDriver orpDriver_;
  TurbidityDriver turbidityDriver_;
  CO2Driver co2Driver_;
  GasSensorDriver gasSensorDriver_;

  SensorReading temperature_;
  SensorReading ph_;
  SensorReading dissolvedOxygen_;
  SensorReading waterLevel_;
  SensorReading ec_;
  SensorReading orp_;
  SensorReading turbidity_;
  SensorReading co2_;
  SensorReading gasSensor_;
};

#endif
