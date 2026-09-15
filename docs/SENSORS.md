# SENSORS

## Phase 1 Sensors

### DS18B20
- Bus: 1-Wire
- Pin mặc định: GPIO4
- Driver: `src/sensors/temperature/ds18b20_driver.*`

### pH Sensor
- ADC: ADS1115 #1 channel A0
- Driver: `src/sensors/ph/ph_sensor_driver.*`
- Calibration mẫu: 2 điểm 7.0 và 4.0

### Dissolved Oxygen
- ADC: ADS1115 #1 channel A1
- Driver: `src/sensors/dissolved_oxygen/do_sensor_driver.*`
- Calibration mẫu: 0 mg/L và 20 mg/L

### Water Level
- Kiểu hiện tại: digital GPIO input
- Driver: `src/sensors/water_level/water_level_driver.*`
- Giả định cấu hình hiện tại: `active_state` là mức báo **nước thấp / low water**
- Giá trị publish: `LOW` khi input bằng `active_state`, ngược lại publish `HIGH`

## Optional / Future Skeletons
- EC/TDS
- ORP
- Turbidity
- CO₂
- Gas sensor

## Example Calibration Procedures

```cpp
// pH example
calibration_low_voltage = 2.50F;  // pH 7.0
calibration_high_voltage = 3.00F; // pH 4.0

// DO example
calibration_low_voltage = 0.00F;  // 0 mg/L
calibration_high_voltage = 2.00F; // 20 mg/L
```
