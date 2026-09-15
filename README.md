# SmartFarm Aquaculture Controller

Firmware ESP32 cho hệ thống quản lý ao nuôi thủy sản với kiến trúc mô-đun, MQTT và Home Assistant.

## Phase Summary

- **Phase 1 / Giai đoạn 1**: DS18B20, pH, DO, Water Level, 8 Relay, MQTT, Home Assistant
- **Phase 2 / Giai đoạn 2**: EC/TDS, ORP, Turbidity (skeleton đã sẵn sàng)
- **Phase 3 / Giai đoạn 3**: CO₂, gas sensors, smart water quality logic (future skeleton)

## Project Layout

```text
src/
├── main.cpp
├── wifi/
├── mqtt/
├── sensors/
├── relays/
├── rules/
├── utils/
└── config/
include/
├── config.h
└── types.h
docs/
├── ARCHITECTURE.md
├── SENSORS.md
├── WIRING.md
├── MQTT_PROTOCOL.md
├── SETUP_GUIDE.md
└── PHASE_ROADMAP.md
```

## Configuration

- Chỉnh WiFi/MQTT trong `src/config/app_config.local.h` (không commit) hoặc dùng placeholder trong `src/config/app_config.h`
- Bật/tắt cảm biến trong `src/config/sensors_config.h`
- Chỉnh relay mapping trong `src/config/relay_config.h`

## Build

```bash
platformio run -e esp32dev
```

## Documentation

Xem thư mục `docs/` để biết chi tiết về kiến trúc, wiring, MQTT và lộ trình phát triển.
