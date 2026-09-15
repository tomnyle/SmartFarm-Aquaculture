# SETUP GUIDE

1. Cài PlatformIO
2. Tạo `src/config/app_config.local.h` và override WiFi/MQTT bằng giá trị thật để tránh commit secrets
3. Mở `src/config/sensors_config.h` để bật/tắt cảm biến đang lắp thực tế
4. Kiểm tra `src/config/relay_config.h` theo sơ đồ chân ESP32 của bạn
5. Build:
   ```bash
   platformio run -e esp32dev
   ```
6. Upload và monitor serial
7. Kết nối Home Assistant tới cùng MQTT broker để nhận discovery entities
