# Home Assistant Setup Guide

## MQTT Discovery

```yaml
mqtt:
  broker: 192.168.100.168
  username: !secret mqtt_user
  password: !secret mqtt_password
  discovery: true
  discovery_prefix: homeassistant
```

Firmware tự publish MQTT Discovery cho:
- 9 sensor chính + status sensor cho từng cảm biến
- 6 relay switches: pump, aerator, circulation, feeder, spare1, spare2
- 2 selectors: mode (AUTO/MANUAL/SCHEDULE/SAFE), profile (Koi/Catfish/Shrimp/Tilapia)

## MQTT Topics

Device topic root:

`smartfarm/aquaculture/{device_id}`

### Sensors

Ví dụ đầy đủ:

```text
smartfarm/aquaculture/{device_id}/sensor/water_temp
smartfarm/aquaculture/{device_id}/sensor/water_temp/min
smartfarm/aquaculture/{device_id}/sensor/water_temp/max
smartfarm/aquaculture/{device_id}/sensor/water_temp/status
```

Tương tự cho:
- `ph` (+ min/max/status)
- `do` (+ min/critical/status)
- `water_level` (+ min/max/status)
- `turbidity` (+ max/status)
- `air_temp` (+ status)
- `humidity` (+ min/max/status)
- `light` (+ min/status)
- `co2` (+ max/status)

### Relays

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

### Mode & Profile

```text
smartfarm/aquaculture/{device_id}/mode
smartfarm/aquaculture/{device_id}/mode/set
smartfarm/aquaculture/{device_id}/profile
smartfarm/aquaculture/{device_id}/profile/set
```

## Lovelace Example

```yaml
type: entities
title: SmartFarm Aquaculture Controller
state_color: true
entities:
  - entity: select.aquaculture_mode
    name: 🎛️ Mode
  - entity: select.aquaculture_profile
    name: 🐟 Profile
  - type: divider
  - entity: sensor.aquaculture_water_temp
    name: 🌡️ Water Temp
  - entity: sensor.aquaculture_water_temp_status
    name: Water Temp Status
  - entity: sensor.aquaculture_ph
    name: 🧪 pH
  - entity: sensor.aquaculture_ph_status
    name: pH Status
  - entity: sensor.aquaculture_do
    name: 💧 Dissolved O₂
  - entity: sensor.aquaculture_do_status
    name: DO Status
  - entity: sensor.aquaculture_water_level
    name: 📊 Water Level
  - entity: sensor.aquaculture_turbidity
    name: 🌫️ Turbidity
  - entity: sensor.aquaculture_air_temp
    name: 🌡️ Air Temp
  - entity: sensor.aquaculture_humidity
    name: 💨 Humidity
  - entity: sensor.aquaculture_light
    name: 💡 Light
  - entity: sensor.aquaculture_co2
    name: 🌀 CO2
  - type: divider
  - entity: switch.aquaculture_pump
    name: 💧 Pump
  - entity: switch.aquaculture_aerator
    name: 💨 Aerator
  - entity: switch.aquaculture_circulation
    name: 🔄 Circulation
  - entity: switch.aquaculture_feeder
    name: 🍖 Feeder
  - entity: switch.aquaculture_spare1
    name: 📌 Spare 1
  - entity: switch.aquaculture_spare2
    name: 📌 Spare 2
```

## Example Automation

```yaml
automation:
  - id: aquaculture_do_warning
    alias: "Aquaculture DO Warning"
    trigger:
      - platform: state
        entity_id: sensor.aquaculture_do_status
        to: "WARNING"
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.aquaculture_aerator
```
