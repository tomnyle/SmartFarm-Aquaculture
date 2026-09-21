# SmartFarm Aquaculture Controller V0.1

ESP32-based aquaculture controller for family pond management with MQTT & Home Assistant integration.

## Features

- **Autonomous Control**: Works independently even without Home Assistant
- **Multi-Species Support**: Profiles for Koi, Catfish, Shrimp, Tilapia, and more
- **4 Operating Modes**: AUTO, MANUAL, SCHEDULE, SAFE
- **Core Sensors**:
  - Water Temperature (DS18B20)
  - pH Level (via ADS1115)
  - Dissolved Oxygen (via ADS1115)
  - Water Level

- **Relay Control** (8-channel):
  - Aerator (Máy sục khí)
  - Water Pump (Bơm cấp nước)
  - Circulation Pump (Bơm tuần hoàn)
  - Feeder (Máy cho ăn)
  - Valve (Van)
  - Light (Đèn)
  - 2x Spare

## Hardware Requirements

- ESP32 DevKitC V4 / ESP-WROOM-32
- DS18B20 Temperature Sensor
- pH Electrode + ADS1115 ADC Module
- Dissolved Oxygen Probe + ADS1115
- Water Level Sensor
- 8-Channel Relay Module
- 5V Power Supply

## Getting Started

### 1. Clone Repository
```bash
git clone https://github.com/tomnyle/SmartFarm-Aquaculture.git
cd SmartFarm-Aquaculture
```

### 2. Configure
Edit `include/app_config.h`:
- WiFi SSID & Password
- MQTT Broker Address
- Device Name & Location

### 3. Build & Upload
```bash
platformio run -e esp32dev -t upload
```

### 4. Monitor Serial Output
```bash
platformio device monitor -b 115200
```

## System Architecture

```
         Home Assistant
              │
             MQTT
              │
      Aquaculture ESP32
              │
    ┌─────────┼─────────┐
    │         │         │
Sensors  Rule Engine  Outputs
    │         │         │
    └─────────┼─────────┘
         Local Controller
```

## Operating Modes

### AUTO Mode
ESP32 automatically controls relays based on sensor readings and active profile thresholds.

### MANUAL Mode
Control relays directly from Home Assistant.

### SCHEDULE Mode
Execute predefined schedules (e.g., feeding times).

### SAFE Mode
Activated when critical errors detected:
- Sensor failures
- Water level too low
- Temperature critical
- DO critical

## Profiles

Each species has predefined parameter ranges:

```json
{
  "name": "shrimp",
  "temperature": { "min": 28, "max": 32 },
  "ph": { "min": 7.5, "max": 8.5 },
  "do": { "min": 5.0 }
}
```

## MQTT Topics

- `smartfarm/aquaculture/state` - System state (publish)
- `smartfarm/aquaculture/sensor` - Sensor readings (publish)
- `smartfarm/aquaculture/output` - Output status (publish)
- `smartfarm/aquaculture/control` - Control commands (subscribe)
- `smartfarm/aquaculture/config` - Configuration (subscribe)
- `smartfarm/aquaculture/condition/*` - Condition/alarm and output-lock states (publish, retained)
- `smartfarm/aquaculture/process/*` - Production phase/readiness/summary states (publish, retained)
- `smartfarm/aquaculture/status` - Device status (publish)

## Documentation

See `/docs` folder for:
- `architecture.md` - System design
- `sensors.md` - Sensor specifications & calibration
- `wiring.md` - Hardware wiring diagram
- `mqtt.md` - MQTT protocol details

## License

MIT License - See LICENSE file

## Author

Tom Nyle (tomnyle) - 2026
