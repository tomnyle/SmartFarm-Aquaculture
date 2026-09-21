# Hướng dẫn test cảm biến an toàn (Firmware V2)

## 1) Trình tự chạy đúng

1. **Test UI/HA không cần cảm biến thật**
   - `BENCH_TEST_MODE=true`
   - `SENSOR_TEST_MODE=false`
   - Relay luôn bị khóa OFF, dữ liệu cảm biến là dữ liệu giả.
2. **Test cảm biến thật nhưng vẫn khóa relay (khuyến nghị khi mới đấu dây)**
   - `BENCH_TEST_MODE=false`
   - `SENSOR_TEST_MODE=true`
   - Firmware đọc cảm biến thật (DS18B20, DHT22, BH1750 nếu init thành công, pH/DO/turbidity analog) nhưng **mọi relay vẫn OFF** dù MQTT gửi ON hay mode AUTO.
3. **Chạy thực tế**
   - `BENCH_TEST_MODE=false`
   - `SENSOR_TEST_MODE=false`
   - Chỉ bật sau khi đã hiệu chuẩn và kiểm tra điện an toàn.

> Lưu ý: không được bật đồng thời `BENCH_TEST_MODE` và `SENSOR_TEST_MODE` (firmware sẽ báo lỗi compile).

## 2) Cảnh báo đấu dây an toàn

- GPIO analog ESP32 chỉ chịu tối đa **3.3V**.
- Tất cả cảm biến phải **chung GND** với ESP32.
- DS18B20 cần điện trở kéo lên **4.7k** giữa DATA và VCC.
- BH1750 dùng nguồn **3.3V** và I2C: SDA=GPIO21, SCL=GPIO22.
- **Ngắt nguồn tải/220V khỏi relay** trong toàn bộ giai đoạn SENSOR_TEST_MODE.
- Không test trực tiếp tải AC khi chưa có mạch bảo vệ/isolator phù hợp.

## 3) Hiệu chuẩn nhanh

- **pH**: chỉnh `PH_NEUTRAL_VOLTAGE`, `PH_SLOPE_VOLT_PER_PH` theo dung dịch chuẩn (ít nhất điểm pH 7 và 1 điểm acid/base), firmware clamp 0..14.
- **DO**: chỉnh `DO_ZERO_VOLTAGE`, `DO_FULL_SCALE_VOLTAGE`, `DO_FULL_SCALE_MG_L`; firmware clamp về dải hợp lý.
- **Turbidity**:
  - Nếu chưa hiệu chuẩn: để `TURBIDITY_CALIBRATED=false`, Home Assistant hiển thị **Raw ADC** (không gắn nhãn NTU).
  - Sau hiệu chuẩn mới bật `TURBIDITY_CALIBRATED=true` và cập nhật các hằng số mapping NTU.

## 4) Điều kiện để Water Level / Current có dữ liệu

- Chỉ online khi:
  - Flag tương ứng bật (`WATER_LEVEL_SENSOR_ENABLED`, `AERATOR_CURRENT_SENSOR_ENABLED`, `PUMP_CURRENT_SENSOR_ENABLED`)
  - Pin tương ứng trong `include/pins.h` khác `-1` và là **ADC1 pin** hợp lệ.
- Nếu chưa thỏa điều kiện, entity sẽ giữ trạng thái unavailable/offline (không giả lập giá trị thật).
