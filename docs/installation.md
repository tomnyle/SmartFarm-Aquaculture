# Installation and Setup

This guide covers firmware flashing, MQTT broker setup, and Home Assistant integration.

## Prerequisites

- ESP32 dev board (tested with `esp32dev` target)
- USB cable for flashing
- [PlatformIO](https://platformio.org/) (recommended)
- MQTT broker (Mosquitto recommended)
- Home Assistant with MQTT integration enabled
- Sensors/relays wired to ESP32 (see `docs/hardware_wiring.md`)

## 1) Configure firmware

Edit:

- `include/app_config.h`

Set at minimum:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `MQTT_BROKER`
- `MQTT_PORT`
- `MQTT_USER`
- `MQTT_PASSWORD`
- `MQTT_CLIENT_ID`

Recommended:

- Change `FW_DEVICE_ID` and `DEVICE_NAME`
- Keep `HA_DISCOVERY_ENABLED true`

## 2) Build and flash

From repository root:

```bash
platformio run -e esp32dev -t upload
```

Open serial monitor:

```bash
platformio device monitor -b 115200
```

Look for:

- Wi-Fi connected
- MQTT connected
- Home Assistant discovery publish logs

## 3) MQTT broker setup (Mosquitto example)

Install Mosquitto and create credentials:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients
sudo mosquitto_passwd -c /etc/mosquitto/passwd smartfarm
```

Minimal `/etc/mosquitto/conf.d/smartfarm.conf`:

```conf
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Restart broker:

```bash
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
```

## 4) Verify MQTT traffic

Subscribe to all project topics:

```bash
mosquitto_sub -h <BROKER_IP> -u <USER> -P <PASSWORD> -t 'smartfarm/aquaculture/#' -v
```

Expected sensor topics:

- `smartfarm/aquaculture/sensor/water_temp`
- `smartfarm/aquaculture/sensor/ph`
- `smartfarm/aquaculture/sensor/do`
- `smartfarm/aquaculture/sensor/co2`
- `smartfarm/aquaculture/sensor/turbidity`
- `smartfarm/aquaculture/sensor/air_temp`
- `smartfarm/aquaculture/sensor/humidity`
- `smartfarm/aquaculture/sensor/light`

Expected output/mode topics:

- `smartfarm/aquaculture/output/pump`
- `smartfarm/aquaculture/output/aerator`
- `smartfarm/aquaculture/output/circulation`
- `smartfarm/aquaculture/output/feeder`
- `smartfarm/aquaculture/config/mode/state`
- `smartfarm/aquaculture/config/species/state`
- `smartfarm/aquaculture/status`

## 5) Home Assistant MQTT Discovery setup

### Recommended: configure MQTT integration in Home Assistant UI

1. Go to **Settings -> Devices & Services -> Add Integration**.
2. Add **MQTT** and enter your broker host/port/credentials.
3. Ensure MQTT Discovery is enabled (default discovery prefix: `homeassistant`).

> Note: for this project, configuring MQTT integration in the Home Assistant UI is the recommended path. YAML-based MQTT configuration can still be used if your deployment is managed that way.

The firmware publishes discovery payloads for:

- 8 sensors (`aquaculture_water_temp`, `aquaculture_ph`, `aquaculture_do`, `aquaculture_co2`, `aquaculture_turbidity`, `aquaculture_air_temp`, `aquaculture_humidity`, `aquaculture_light`)
- 4 switches (`aquaculture_pump`, `aquaculture_aerator`, `aquaculture_circulation`, `aquaculture_feeder`)
- 2 selects (`aquaculture_mode`, `aquaculture_species`)

In Home Assistant, these appear as entities such as:

- `sensor.aquaculture_water_temp`, `sensor.aquaculture_ph`, `sensor.aquaculture_do`
- `switch.aquaculture_pump`, `switch.aquaculture_aerator`
- `select.aquaculture_mode`, `select.aquaculture_species`

## 6) Send a control command (manual test)

```bash
mosquitto_pub -h <BROKER_IP> -u <USER> -P <PASSWORD> -t 'smartfarm/aquaculture/control/pump/set' -m 'ON'
```

Other command topics:

- `smartfarm/aquaculture/control/aerator/set`
- `smartfarm/aquaculture/control/circulation/set`
- `smartfarm/aquaculture/control/feeder/set`
- `smartfarm/aquaculture/config/mode/set`
- `smartfarm/aquaculture/config/species/set`

Species selection note (important):

- Dedicated rule profiles: `Cá Chép`, `Rô Phi`, `Cá Tra`, `Tôm Thẻ`
- `Rô Phi` can be set manually on `smartfarm/aquaculture/config/species/set` even though it is not in the discovery dropdown options
- Discovery-exposed labels `Koi`, `Cá Trắm`, `Cá Lóc`, `Tôm Sú`, `Tilapia` are accepted on `.../config/species/set` but currently use fallback `Rô Phi` thresholds
- `smartfarm/aquaculture/config/species/state` echoes the selected label, not the resolved internal profile name

## Troubleshooting

- **No MQTT connection**: verify broker IP/credentials in `include/app_config.h`
- **No HA entities**: ensure MQTT Discovery is enabled and broker is shared between HA and ESP32
- **No sensor updates**: check serial output and wiring in `docs/hardware_wiring.md`
- **Commands not applied**:
  - Relay control topics (`smartfarm/aquaculture/control/.../set`) require `ON` / `OFF`
  - Mode config topic (`smartfarm/aquaculture/config/mode/set`) requires mode string: `AUTO`, `MANUAL`, `SCHEDULE`, or `SAFE`
  - Species config topic (`smartfarm/aquaculture/config/species/set`) requires species label text (for example `Cá Chép` or `Tôm Thẻ`)
  - Discovery currently exposes species labels without dedicated rule profiles (`Koi`, `Cá Trắm`, `Cá Lóc`, `Tôm Sú`, `Tilapia`); those labels are accepted but use fallback `Rô Phi` thresholds
