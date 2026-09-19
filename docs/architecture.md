# SmartAquaculture Architecture V1

## 1. Deployment target

V1 targets a **single family pond around 200 m²** with one ESP32 controller that can continue protecting the pond when the network is unavailable.

## 2. V1 single-node architecture

```text
Sensors ─┐
         ├── ESP32 SmartAquaculture V1 ── MQTT ── Home Assistant ── Recorder / InfluxDB / Grafana
Outputs ─┘
```

### Sensor side
- DO (primary safety signal)
- Water temperature (DS18B20)
- pH
- Water level
- Optional advisory hooks: turbidity, CO2, light, air temp/humidity, aerator current, pump current

### Control side
- Aerator 1
- Aerator 2
- Pump
- Circulation
- Feeder
- Alarm beacon / buzzer

### Safety side
- Sensor validity / stale detection
- DO-first fail-safe control
- Water-level critical lockout
- Pump max runtime protection
- Feeder runtime limit
- Current-monitoring fault hooks

## 3. Future 2-node architecture

When the pond grows or uptime becomes more critical, split the design:

```text
ESP32 Sensor Node            ESP32 Control Node
- DO                         - Aerator 1 / 2
- pH                         - Pump
- Temperature                - Feeder
- Water level                - Alarm
        \                    /
         \---- MQTT bus ----/
                 |
          Home Assistant
```

This separation reduces the chance that a sensor-side failure also stops protective outputs.

## 4. State machine

### AUTO
- Default operating mode.
- Uses local rules and species advisory limits.
- May start aeration, circulation, or refill pump automatically.

### MANUAL
- Accepts MQTT operator commands.
- Still cannot violate hard safety rules.
- Manual override automatically times out and returns to AUTO.

### SCHEDULE
- Reserved for timer-based automation such as feeding windows.
- In V1 it behaves like AUTO with room for future schedules.

### SAFE
- Entered automatically on severe but non-emergency conditions such as:
  - DO sensor fault / stale data
  - DO critical
  - water level critical
  - temperature critical
- Conservative response: protect pond first, disable risky actions, lock feeder.

### EMERGENCY
- Highest priority.
- Triggered by DO emergency level or operator-forced emergency mode.
- Both aerators ON, alarm ON, feeder locked.

## 5. Data flow

```text
ESP32 sensor read
  -> local validation / stale detection
  -> local rule engine decides outputs
  -> relay state changes
  -> MQTT telemetry and events
  -> Home Assistant entities
  -> Recorder / InfluxDB / Grafana history
```

## 6. Design notes

- V1 keeps the **decision engine on the ESP32**.
- Home Assistant is for visibility, acknowledgements, dashboards, and higher-level automation.
- The pond must remain protected if Wi-Fi or MQTT is down.
