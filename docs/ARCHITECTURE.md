# ARCHITECTURE

## Overview

Firmware được chia theo module để có thể bật/tắt cảm biến từ `src/config/sensors_config.h` mà không phải sửa core logic.

## Modules

- `wifi/`: quản lý kết nối WiFi
- `mqtt/`: MQTT topics, publish state, Home Assistant discovery
- `sensors/`: driver độc lập cho từng cảm biến
- `relays/`: quản lý 8 relay
- `rules/`: profile loài nuôi và automation rules
- `utils/`: logging và calibration helpers
- `config/`: toàn bộ compile-time configuration

## Runtime Flow

1. Boot ESP32
2. Init relay, I2C, 1-Wire
3. Runtime check config từng cảm biến; sensor disabled sẽ bỏ qua init
4. Đọc cảm biến theo chu kỳ
5. Rule engine đánh giá theo species profile khi mode = AUTO
6. Nếu mode = SAFE thì firmware ép relay về trạng thái an toàn (aerator ON, relay còn lại OFF)
7. Publish `state`, `sensor/all`, sensor topics và relay topics qua MQTT

## Extensibility

Driver mới chỉ cần:
1. thêm config struct trong `sensors_config.h`
2. tạo driver trong `src/sensors/<module>/`
3. publish topic mới nếu cần
4. giữ nguyên core flow trong `main.cpp`
