# SETUP GUIDE

1. Cài PlatformIO
2. Mở `src/config/app_config.h` và thay WiFi/MQTT placeholders bằng giá trị thật
3. Mở `src/config/sensors_config.h` để bật/tắt cảm biến đang lắp thực tế
4. Kiểm tra `src/config/relay_config.h` theo sơ đồ chân ESP32 của bạn
5. Build:
   ```bash
   platformio run -e esp32dev
   ```
6. Upload và monitor serial
7. Kết nối Home Assistant tới cùng MQTT broker để nhận discovery entities
