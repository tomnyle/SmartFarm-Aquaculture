# Architecture

## Phase 1 modules
- `src/sensors`: DS18B20, pH, DO, water level drivers and sensor manager
- `src/relays`: 8 relay definitions and relay manager
- `src/rules`: species profiles, condition evaluation, AUTO logic
- `src/modes`: AUTO, MANUAL, SCHEDULE, SAFE mode behavior
- `src/mqtt`: topic generation, Home Assistant discovery, MQTT manager
- `src/wifi`: WiFi reconnect and NTP bootstrap
- `src/system`: runtime status and error tracking

## Runtime flow
1. Connect WiFi
2. Connect MQTT and publish Home Assistant discovery
3. Poll enabled sensors
4. Evaluate conditions against active profile
5. Apply relays based on selected mode
6. Publish individual topics plus consolidated JSON state
