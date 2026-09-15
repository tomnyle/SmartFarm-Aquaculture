# SmartFarm Aquaculture

ESP32 firmware for aquaculture pond monitoring and control with MQTT and Home Assistant auto-discovery.

## What it does

- Publishes real-time pond/environment sensors over MQTT
- Exposes relay outputs (pump, aerator, circulation, feeder) via MQTT
- Auto-registers Home Assistant entities through MQTT Discovery
- Runs a species-based AUTO rule engine for water quality control
- Supports mode selection: `AUTO`, `MANUAL`, `SCHEDULE`, `SAFE`

## Current sensors and outputs

### Sensors
- Water temperature (DS18B20)
- pH (analog)
- Dissolved oxygen (analog)
- CO2 (analog)
- Turbidity (analog)
- Air temperature + humidity (DHT22)
- Light (BH1750 over I2C)

### Outputs
- Pump
- Aerator
- Circulation
- Feeder

## Quick start

1. Install PlatformIO.
2. Configure Wi-Fi and MQTT in `include/app_config.h`.
3. Build and flash:

```bash
platformio run -e esp32dev -t upload
platformio device monitor -b 115200
```

4. Configure Home Assistant MQTT integration (UI recommended).  
   Discovery entities appear after the ESP32 has connected to MQTT at least once and published retained discovery topics.
   - See `/docs/home_assistant_setup.md` and `/docs/installation.md`
5. Verify entities appear under the discovered SmartFarm aquaculture device (name shown in Home Assistant may vary by firmware/device settings).

> Security note: never commit real Wi-Fi/MQTT credentials. Keep deployment credentials in your local working copy only.

## MQTT topic convention

All topics are under:

- `smartfarm/aquaculture/sensor/...`
- `smartfarm/aquaculture/output/...`
- `smartfarm/aquaculture/control/.../set`
- `smartfarm/aquaculture/config/...`
- `smartfarm/aquaculture/status`

See full topic tables in `/docs/installation.md` and `/docs/home_assistant_setup.md`.

## Documentation

- [docs/installation.md](docs/installation.md)
- [docs/hardware_wiring.md](docs/hardware_wiring.md)
- [docs/rule_engine.md](docs/rule_engine.md)
- [docs/home_assistant_setup.md](docs/home_assistant_setup.md)
- [docs/architecture.md](docs/architecture.md)
- [docs/sensors.md](docs/sensors.md)
