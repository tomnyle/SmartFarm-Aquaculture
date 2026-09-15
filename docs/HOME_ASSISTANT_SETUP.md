# Home Assistant Setup

The firmware publishes MQTT discovery payloads under `homeassistant/...`, so sensors, switches, and selectors can be auto-created.

## Exposed entities
- Sensors: temperature, pH, DO, water level
- Status sensors: temperature condition, pH condition, DO condition, water level condition, system state, system error
- Switches: aerator, pump, circulation, feeder, valve, light, spare1, spare2
- Selectors: mode, profile

## Example dashboard card
Home Assistant may slug entity IDs differently based on your installation, so treat the IDs below as examples and adjust them to the discovered entities shown in Home Assistant.

```yaml
type: entities
title: SmartFarm Aquaculture
entities:
  - <temperature_entity_id>
  - <ph_entity_id>
  - <do_entity_id>
  - <water_level_entity_id>
  - <aerator_switch_entity_id>
  - <pump_switch_entity_id>
  - <mode_select_entity_id>
  - <profile_select_entity_id>
```
