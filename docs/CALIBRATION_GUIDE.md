# Calibration Guide

## DS18B20
Normally does not need calibration.

## pH
Adjust the voltage-to-pH conversion in `src/sensors/ph/ph_sensor_driver.cpp` after measuring your buffer solutions.

## Dissolved Oxygen
Tune the voltage scale in `src/sensors/dissolved_oxygen/do_sensor_driver.cpp` for your probe and amplifier.

## Water Level
Map the analog range to 0-100% in `src/sensors/water_level/water_level_driver.cpp` if your sensor range differs.
