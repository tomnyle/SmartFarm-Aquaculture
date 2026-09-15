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

- `Koi`
- `Cá Trắm`
- `Cá Chép`
- `Cá Tra`
- `Cá Lóc`
- `Tôm Thẻ`
- `Tôm Sú`
- `Tilapia`

Rule-engine profile mapping currently supports: `Cá Chép`, `Rô Phi`, `Cá Tra`, `Tôm Thẻ`. Other selected labels currently fall back to `Rô Phi` thresholds.

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
- **Unavailable entities**: ensure ESP32 is online and publishing retained topics
- **Wrong values**: verify pin mapping and analog sensor scaling
- **Commands ignored**: use exact `ON`/`OFF` payloads and correct `.../set` topic
