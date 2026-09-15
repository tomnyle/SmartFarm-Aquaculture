# Home Assistant Setup

The firmware publishes MQTT discovery payloads under `homeassistant/...`, so sensors, switches, and selectors can be auto-created.

## Exposed entities
- Sensors: temperature, pH, DO, water level
- Condition sensors: temperature, pH, DO, water level
- Switches: aerator, pump, circulation, feeder, valve, light, spare1, spare2
- Selectors: mode, profile
- Text sensors: system state, error

## Example dashboard card
```yaml
type: entities
title: SmartFarm Aquaculture
entities:
  - sensor.esp32_aquaculture_001_temperature
  - sensor.esp32_aquaculture_001_ph
  - sensor.esp32_aquaculture_001_do
  - sensor.esp32_aquaculture_001_water_level
  - switch.esp32_aquaculture_001_aerator
  - switch.esp32_aquaculture_001_pump
  - select.esp32_aquaculture_001_mode
  - select.esp32_aquaculture_001_profile
```
