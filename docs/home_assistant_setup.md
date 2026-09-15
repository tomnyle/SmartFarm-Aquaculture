# Home Assistant Setup

## Requirements

- Home Assistant with MQTT integration
- MQTT broker reachable by both Home Assistant and ESP32
- Firmware configured with the same broker credentials

## 1) Add MQTT integration (recommended)

1. Go to **Settings -> Devices & Services -> Add Integration**.
2. Add **MQTT** and set broker host/port/credentials.
3. Keep discovery enabled (default prefix `homeassistant`).

> Note: UI-based MQTT integration is recommended for this project. YAML-based MQTT configuration can still be used if your Home Assistant deployment is managed that way.

## 2) Flash and boot ESP32

After MQTT connection, firmware publishes retained discovery payloads to:

- `homeassistant/sensor/.../config`
- `homeassistant/switch/.../config`
- `homeassistant/select/.../config`

## 3) Verify discovered entities

Expected entities:

### Sensors
- `sensor.aquaculture_water_temp`
- `sensor.aquaculture_ph`
- `sensor.aquaculture_do`
- `sensor.aquaculture_co2`
- `sensor.aquaculture_turbidity`
- `sensor.aquaculture_air_temp`
- `sensor.aquaculture_humidity`
- `sensor.aquaculture_light`

### Switches
- `switch.aquaculture_pump`
- `switch.aquaculture_aerator`
- `switch.aquaculture_circulation`
- `switch.aquaculture_feeder`

### Selects
- `select.aquaculture_mode`
- `select.aquaculture_species`

Current `select.aquaculture_species` options published by firmware discovery:

- Dedicated mapped profiles: `Cá Chép`, `Cá Tra`, `Tôm Thẻ`
- Fallback-to-`Rô Phi` aliases: `Koi`, `Cá Trắm`, `Cá Lóc`, `Tôm Sú`, `Tilapia`

Rule-engine profile mapping currently supports: `Cá Chép`, `Rô Phi`, `Cá Tra`, `Tôm Thẻ`.  
`Rô Phi` is currently an internal/default profile and is not included as a direct discovery option.
You can still set `Rô Phi` manually by publishing directly to `smartfarm/aquaculture/config/species/set`.
`smartfarm/aquaculture/config/species/state` echoes the selected label, not the resolved internal profile name; fallback-to-`Rô Phi` behavior is inferred from mapping rules above.
So from Home Assistant alone, selected species label and applied threshold profile are not always a 1:1 match.

## 4) Add dashboard card

```yaml
type: entities
title: Aquaculture Controller
state_color: true
entities:
  - sensor.aquaculture_water_temp
  - sensor.aquaculture_ph
  - sensor.aquaculture_do
  - sensor.aquaculture_co2
  - sensor.aquaculture_turbidity
  - sensor.aquaculture_air_temp
  - sensor.aquaculture_humidity
  - sensor.aquaculture_light
  - select.aquaculture_mode
  - select.aquaculture_species
  - switch.aquaculture_pump
  - switch.aquaculture_aerator
  - switch.aquaculture_circulation
  - switch.aquaculture_feeder
```

## 5) Validate topic flow

Subscribe:

```bash
mosquitto_sub -h <BROKER_IP> -u <USER> -P <PASSWORD> -t 'smartfarm/aquaculture/#' -v
```

Control test:

```bash
mosquitto_pub -h <BROKER_IP> -u <USER> -P <PASSWORD> -t 'smartfarm/aquaculture/control/aerator/set' -m 'ON'
```

## Troubleshooting

- **No entities discovered**: check MQTT Discovery enabled, then restart MQTT integration/Home Assistant
- **Still no entities after restart**: retained discovery messages may be missing (for example broker reset/cleanup); reboot ESP32 or force MQTT reconnect to republish discovery config topics
- **Unavailable entities**: ensure ESP32 is online, connected to Wi-Fi/MQTT, and still publishing state updates
- **Wrong values**: verify pin mapping and analog sensor scaling
- **Commands ignored**:
  - Switch topics use `ON` / `OFF`
  - `config/mode/set` expects `AUTO`, `MANUAL`, `SCHEDULE`, or `SAFE`
  - `config/species/set` expects a species label string
  - Some discovered species labels currently fall back to default `Rô Phi` thresholds (no dedicated profile mapping yet)
