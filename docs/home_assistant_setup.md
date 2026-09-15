# Home Assistant Setup Guide

## MQTT Discovery

Enable MQTT discovery in Home Assistant:

```yaml
mqtt:
  broker: 192.168.100.168
  username: !secret mqtt_user
  password: !secret mqtt_password
  discovery: true
  discovery_prefix: homeassistant
```

The firmware publishes MQTT discovery for:
- 9 sensors: water temperature, pH, dissolved oxygen, water level, turbidity, air temperature, humidity, light, CO2
- 9 sensor status entities (`NORMAL`, `WARNING`, `CRITICAL`, `UNAVAILABLE`)
- 6 relay switches: pump, aerator, circulation, feeder, spare1, spare2
- 2 selects: mode and profile
- Controller status and error entities

All runtime topics are namespaced by device id:

```text
smartfarm/aquaculture/ESP32_AQUACULTURE_001/
```

## Modes

- `AUTO`: relay control follows sensor thresholds and the selected profile
- `MANUAL`: Home Assistant switch commands directly drive all 6 relays
- `SCHEDULE`: firmware runs fixed feeder/pump/circulation/aerator intervals
- `SAFE`: enabled automatically on critical faults or manually from Home Assistant

## Sensor Topics

Each sensor publishes:

```text
smartfarm/aquaculture/{device_id}/sensor/{name}           -> current value
smartfarm/aquaculture/{device_id}/sensor/{name}/status    -> NORMAL|WARNING|CRITICAL|UNAVAILABLE
smartfarm/aquaculture/{device_id}/sensor/{name}/min       -> min threshold (when defined)
smartfarm/aquaculture/{device_id}/sensor/{name}/max       -> max threshold (when defined)
smartfarm/aquaculture/{device_id}/sensor/{name}/critical  -> critical threshold (when defined)
smartfarm/aquaculture/{device_id}/sensor/{name}/attributes -> JSON attributes for HA
```

Primary sensors:

- `water_temp` → thresholds from `TEMP_ALERT_LOW` / `TEMP_ALERT_HIGH`
- `ph` → thresholds from `PH_ALERT_LOW` / `PH_ALERT_HIGH`
- `do` → thresholds from `DO_ALERT_LOW` / `DO_CRITICAL_LOW`
- `water_level` → default `20-100%`
- `turbidity` → default max `5 NTU`
- `air_temp` → status only
- `humidity` → default `50-80%`
- `light` → default min `500 lux`
- `co2` → threshold from `CO2_ALERT_HIGH`

## Relay Topics

```text
smartfarm/aquaculture/{device_id}/relay/pump/state
smartfarm/aquaculture/{device_id}/relay/pump/command/set
smartfarm/aquaculture/{device_id}/relay/aerator/state
smartfarm/aquaculture/{device_id}/relay/aerator/command/set
smartfarm/aquaculture/{device_id}/relay/circulation/state
smartfarm/aquaculture/{device_id}/relay/circulation/command/set
smartfarm/aquaculture/{device_id}/relay/feeder/state
smartfarm/aquaculture/{device_id}/relay/feeder/command/set
smartfarm/aquaculture/{device_id}/relay/spare1/state
smartfarm/aquaculture/{device_id}/relay/spare1/command/set
smartfarm/aquaculture/{device_id}/relay/spare2/state
smartfarm/aquaculture/{device_id}/relay/spare2/command/set
```

## Mode, Profile, Status Topics

```text
smartfarm/aquaculture/{device_id}/mode
smartfarm/aquaculture/{device_id}/mode/set
smartfarm/aquaculture/{device_id}/profile
smartfarm/aquaculture/{device_id}/profile/set
smartfarm/aquaculture/{device_id}/state
smartfarm/aquaculture/{device_id}/status
smartfarm/aquaculture/{device_id}/error
smartfarm/aquaculture/{device_id}/availability
```

## Lovelace Dashboard Example

```yaml
type: entities
title: SmartFarm Aquaculture
state_color: true
entities:
  - entity: select.esp32_aquaculture_001_mode
    name: 🎛️ Mode
  - entity: select.esp32_aquaculture_001_profile
    name: 🐟 Profile
  - entity: sensor.esp32_aquaculture_001_controller_status
    name: ✅ Status
  - entity: sensor.esp32_aquaculture_001_controller_error
    name: ⚠️ Error

  - type: divider

  - entity: sensor.esp32_aquaculture_001_water_temp
    name: 🌡️ Water Temp
  - entity: sensor.esp32_aquaculture_001_water_temp_status
    name: Water Temp Status
  - entity: sensor.esp32_aquaculture_001_ph
    name: 🧪 pH
  - entity: sensor.esp32_aquaculture_001_ph_status
    name: pH Status
  - entity: sensor.esp32_aquaculture_001_do
    name: 💧 Dissolved O₂
  - entity: sensor.esp32_aquaculture_001_do_status
    name: DO Status
  - entity: sensor.esp32_aquaculture_001_water_level
    name: 📊 Water Level
  - entity: sensor.esp32_aquaculture_001_water_level_status
    name: Water Level Status
  - entity: sensor.esp32_aquaculture_001_turbidity
    name: 🌫️ Turbidity
  - entity: sensor.esp32_aquaculture_001_turbidity_status
    name: Turbidity Status
  - entity: sensor.esp32_aquaculture_001_air_temp
    name: 🌡️ Air Temp
  - entity: sensor.esp32_aquaculture_001_air_temp_status
    name: Air Temp Status
  - entity: sensor.esp32_aquaculture_001_humidity
    name: 💨 Humidity
  - entity: sensor.esp32_aquaculture_001_humidity_status
    name: Humidity Status
  - entity: sensor.esp32_aquaculture_001_light
    name: 💡 Light
  - entity: sensor.esp32_aquaculture_001_light_status
    name: Light Status
  - entity: sensor.esp32_aquaculture_001_co2
    name: 🌀 CO₂
  - entity: sensor.esp32_aquaculture_001_co2_status
    name: CO₂ Status

  - type: divider

  - entity: switch.esp32_aquaculture_001_pump
    name: 💧 Pump
  - entity: switch.esp32_aquaculture_001_aerator
    name: 💨 Aerator
  - entity: switch.esp32_aquaculture_001_circulation
    name: 🔄 Circulation
  - entity: switch.esp32_aquaculture_001_feeder
    name: 🍖 Feeder
  - entity: switch.esp32_aquaculture_001_spare1
    name: 📌 Spare 1
  - entity: switch.esp32_aquaculture_001_spare2
    name: 📌 Spare 2
```

The main sensor entities also expose threshold attributes (`min`, `max`, `critical_low`, `critical_high`, `status`, `profile`) so custom cards can render the `[min-max]` information directly.

## MQTT Checks

Subscribe to verify all topics:

```bash
mosquitto_sub -h 192.168.100.168 -u homer -P '<password>' -v -t 'smartfarm/aquaculture/ESP32_AQUACULTURE_001/#'
```

Send a manual command:

```bash
mosquitto_pub -h 192.168.100.168 -u homer -P '<password>' -t 'smartfarm/aquaculture/ESP32_AQUACULTURE_001/relay/pump/command/set' -m 'ON'
```
