# MQTT Protocol

Base prefix: `smartfarm/aquaculture/{device_id}`

## Publish
- `state`
- `sensor/temperature`
- `sensor/ph`
- `sensor/do`
- `sensor/water_level`
- `relay/{relay_name}` for all 8 relays
- `system/mode`
- `system/profile`
- `system/state`
- `conditions/temperature`
- `conditions/ph`
- `conditions/do`
- `conditions/water_level`
- `status/availability`
- `status/error`

## Subscribe
- `control/relay/{relay_name}/set`
- `control/mode/set`
- `control/profile/set`
- `control/schedule/set`
