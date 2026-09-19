# MQTT Topic Specification

## Topic table

| Topic | Direction | Payload | Notes |
|---|---|---|---|
| `smartfarm/aquaculture/sensor/water_temp` | publish | number | retained |
| `smartfarm/aquaculture/sensor/ph` | publish | number | retained |
| `smartfarm/aquaculture/sensor/do` | publish | number | retained |
| `smartfarm/aquaculture/sensor/water_level` | publish | number | retained |
| `smartfarm/aquaculture/sensor/aerator_current` | publish | number | retained |
| `smartfarm/aquaculture/sensor/pump_current` | publish | number | retained |
| `smartfarm/aquaculture/output/aerator_1` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/output/aerator_2` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/output/pump` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/output/feeder` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/output/alarm` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/control/aerator_1/set` | subscribe | `ON` / `OFF` | manual control |
| `smartfarm/aquaculture/control/aerator_2/set` | subscribe | `ON` / `OFF` | manual control |
| `smartfarm/aquaculture/control/pump/set` | subscribe | `ON` / `OFF` | manual control |
| `smartfarm/aquaculture/control/feeder/set` | subscribe | `ON` / `OFF` | manual control |
| `smartfarm/aquaculture/control/alarm/set` | subscribe | `ON` / `OFF` | manual control |
| `smartfarm/aquaculture/config/mode/set` | subscribe | mode string | AUTO / MANUAL / SCHEDULE / SAFE / EMERGENCY |
| `smartfarm/aquaculture/config/mode/state` | publish | mode string | retained |
| `smartfarm/aquaculture/config/species/state` | publish | species string | retained |
| `smartfarm/aquaculture/event/alarm` | publish | JSON | warning/critical/emergency events |
| `smartfarm/aquaculture/event/sensor_fault` | publish | JSON | stale/invalid sensor faults |
| `smartfarm/aquaculture/event/device_fault` | publish | JSON | timeout/current faults |
| `smartfarm/aquaculture/status` | publish | `online` / `offline` | retained LWT |
| `smartfarm/aquaculture/status/heartbeat` | publish | JSON | non-retained periodic heartbeat |
| `smartfarm/aquaculture/status/safety_state` | publish | string | retained safety level |
| `smartfarm/aquaculture/status/alarm_text` | publish | string | retained active message |
| `smartfarm/aquaculture/status/safety_active` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/status/emergency_active` | publish | `ON` / `OFF` | retained |
| `smartfarm/aquaculture/controller/state` | publish | JSON | compact controller summary |

## Payload examples

### Sensor payload
```text
smartfarm/aquaculture/sensor/do
5.82
```

### Heartbeat payload
```json
{
  "uptime_s": 10234,
  "wifi": true,
  "mqtt": true,
  "mode": "AUTO",
  "safety_level": "NORMAL"
}
```

### Alarm payload
```json
{
  "severity": "CRITICAL",
  "event": "alarm",
  "message": "DO below critical threshold - safe mode aeration active",
  "mode": "SAFE",
  "requires_ack": true,
  "actions": ["inspect_feeder_lock", "check_aeration"]
}
```

## Retained vs non-retained

- **Retained**: sensor last values, output states, current mode, species, safety state, alarm flags.
- **Non-retained**: heartbeat and event notifications.

## LWT and heartbeat

- `smartfarm/aquaculture/status` is used as the MQTT Last Will and Testament topic.
- `heartbeat` confirms that the device is still looping even if no alarm is active.
