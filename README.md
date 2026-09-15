# SmartFarm Aquaculture Firmware

ESP32 aquaculture controller firmware with modular Phase 1 support for DS18B20, pH, dissolved oxygen, water level, 8 relays, MQTT, and Home Assistant discovery.

## Implemented now (Phase 1)
- DS18B20 water temperature
- pH via ADS1115
- Dissolved oxygen via ADS1115
- Water level via GPIO ADC
- 8-channel relay manager
- MQTT publish/subscribe topics under `smartfarm/aquaculture/{device_id}`
- Home Assistant MQTT Discovery
- 4 operating modes: AUTO, MANUAL, SCHEDULE, SAFE
- Species profiles: Koi, Catfish, Shrimp, Tilapia

## Project layout
```text
src/
├── main.cpp
├── config/
├── wifi/
├── mqtt/
├── sensors/
├── relays/
├── rules/
├── modes/
├── system/
└── utils/
include/
├── version.h
└── constants.h
```

## Configure
Edit `/home/runner/work/SmartFarm-Aquaculture/SmartFarm-Aquaculture/src/config/app_config.h` and replace the placeholder WiFi and MQTT values with your environment settings.

## Build
```bash
platformio run -e esp32dev
```

## Upload
```bash
platformio run -e esp32dev -t upload
```

## Serial monitor
```bash
platformio device monitor -b 115200
```

## Documentation
See the `docs/` directory for architecture, MQTT, Home Assistant, profiles, calibration, and roadmap details.
