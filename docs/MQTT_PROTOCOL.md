# MQTT PROTOCOL

Base topic:

```text
smartfarm/aquaculture/{device_id}/
```

## Published
- `state`
- `sensor/all`
- `sensor/temperature`
- `sensor/ph`
- `sensor/do`
- `sensor/water_level`
- `sensor/ec`
- `sensor/orp`
- `relay/all`
- `relay/{name}`
- `status/error`

## Subscribed
- `control/relay/{name}`
- `control/mode`

## Home Assistant
MQTT manager tự publish discovery config cho cảm biến Phase 1 và 8 relay.
