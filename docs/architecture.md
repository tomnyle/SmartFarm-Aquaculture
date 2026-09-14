# SmartFarm Aquaculture Controller - Architecture

## System Overview

The Aquaculture Controller is designed to manage family pond conditions autonomously. It operates independently from Home Assistant but integrates with it via MQTT for monitoring and remote control.

## Core Principles

1. **Autonomy**: Works without network connection
2. **Simplicity**: Focus on essential measurements only
3. **Safety**: Conservative approach to automation (alert, don't auto-fix)
4. **Modularity**: Easy to extend with more sensors/outputs

## Hardware Architecture

```
    ESP32 Microcontroller
    ├── 1-Wire Bus (GPIO4)
    │   └── DS18B20 Temperature Sensor
    ├── I2C Bus (GPIO21/22)
    │   ├── ADS1115 #1 (pH & DO)
    │   └── ADS1115 #2 (optional, future expansion)
    ├── Analog Input (GPIO34)
    │   └── Water Level Sensor
    └── GPIO Digital Outputs (GPIO32-27, 14, 12-13)
        ├── Relay 1: Aerator
        ├── Relay 2: Water Pump
        ├── Relay 3: Circulation
        ├── Relay 4: Feeder
        ├── Relay 5: Valve
        ├── Relay 6: Light
        ├── Relay 7: Spare 1
        └── Relay 8: Spare 2
```

## Software Architecture

### Layer 1: Hardware Abstraction
- Sensor drivers (temperature, pH, DO, level)
- Relay drivers
- ADC interface (ADS1115)
- 1-Wire interface

### Layer 2: Core Services
- System state management
- Configuration management
- Rule engine
- Scheduler

### Layer 3: Communication
- WiFi stack
- MQTT client
- Home Assistant discovery

### Layer 4: Application Logic
- Aquaculture-specific profiles
- Operating mode management
- Error handling

## Control Flow

```
┌─────────────────────┐
│  Read Sensors       │
│  (30s interval)     │
└──────────┬──────────┘
           │
           v
┌─────────────────────┐
│  Evaluate Rules     │
│  (5s interval)      │
└──────────┬──────────┘
           │
           v
┌─────────────────────┐
│  Apply Outputs      │
└──────────┬──────────┘
           │
           v
┌─────────────────────┐
│  Publish MQTT       │
│  (60s interval)     │
└─────────────────────┘
```

## Operating Modes

### AUTO Mode
- Reads sensor values
- Evaluates rules against active profile
- Automatically controls outputs
- Debounced to prevent relay chatter

### MANUAL Mode
- Ignores rules
- Accepts direct commands from MQTT
- Used for testing and emergency control

### SCHEDULE Mode
- Executes predefined time-based commands
- Typically for feeding schedules
- Can be combined with AUTO mode

### SAFE Mode
- Activated on critical errors:
  - Multiple sensor failures
  - Water level below critical threshold
  - Temperature exceeding safe limits
  - DO below critical level
- Default safe state:
  - Aerator: ON
  - All other outputs: OFF
  - Periodic alarm notification

## Safety Features

1. **Sensor Validation**: Sanity checks on all sensor readings
2. **Debouncing**: Prevents rapid relay switching
3. **Minimum On/Off Times**: Protects equipment from rapid cycling
4. **Watchdog Timer**: Resets system on hangup
5. **Safe Mode Fallback**: Enters protective state on errors
6. **No Chemical Dosing**: Phase 1 only alerts on pH issues

## MQTT Integration

All sensor data published to MQTT at configured interval (default 60s):

```json
{
  "device_id": "aquaculture-001",
  "timestamp": 1694726400,
  "mode": "AUTO",
  "status": "running",
  "sensors": {
    "temperature": 27.5,
    "ph": 7.8,
    "do": 6.2,
    "level": 85.3
  },
  "outputs": {
    "aerator": true,
    "water_pump": false,
    "circulation": false,
    "feeder": false
  },
  "profile": "shrimp"
}
```

## Error Handling

| Error | Response | Recovery |
|-------|----------|----------|
| Sensor failure | Log error, use last valid value | Retry next cycle |
| MQTT disconnect | Continue local operation | Auto-reconnect |
| Water level low | Alert, disable pump | Manual intervention |
| Temperature extreme | Alert, enable safety devices | Check heating/cooling |
| DO critical | Enable aerator, alert | Check aerator functionality |

## Future Expansion

Phase 2+ additions:
- ORP (Oxidation-Reduction Potential)
- Salinity
- EC (Electrical Conductivity)
- Turbidity
- NH3/NH4, NO2, NO3
- Multiple pond support
- Advanced scheduling
- Data logging to cloud
