# Home Assistant Setup Guide

## Requirement
- Home Assistant running (any version with MQTT support)
- MQTT Broker (Mosquitto recommended)
- Network connection to ESP32

## Step 1: Enable MQTT Discovery

Add to `configuration.yaml`:

```yaml
mqtt:
  broker: 192.168.1.100  # Your MQTT broker IP
  username: !secret mqtt_user
  password: !secret mqtt_password
  discovery: true
  discovery_prefix: homeassistant
```

## Step 2: Automatic Discovery (Recommended)

✅ **ESP32 will automatically register all sensors & switches!**

No need to manually add anything. Just:
1. Upload firmware to ESP32
2. Wait 30 seconds for MQTT connection
3. Check Home Assistant - entities appear automatically!

## Step 3: Create Dashboard

### Option A: Auto-Generated (Recommended)
1. Go to Home Assistant → Settings → Devices & Services
2. Find "Aquaculture Controller" device
3. Click → All entities created automatically

### Option B: Manual Lovelace Card

Create `aquaculture_dashboard.yaml`:

```yaml
type: entities
title: Aquaculture Pond
state_color: true
entities:
  - entity: sensor.aquaculture_temperature
    name: 🌡️ Temperature
    icon: mdi:thermometer
  
  - entity: sensor.aquaculture_ph
    name: 🧪 pH Level
    icon: mdi:test-tube
  
  - entity: sensor.aquaculture_do
    name: 💧 Dissolved Oxygen
    icon: mdi:water
  
  - entity: sensor.aquaculture_water_level
    name: 📊 Water Level
    icon: mdi:water-percent
  
  - type: divider
  
  - entity: select.aquaculture_species
    name: 🐟 Species
    icon: mdi:fish
  
  - entity: select.aquaculture_mode
    name: ⚙️ Mode
    icon: mdi:cog
  
  - type: divider
  
  - entity: switch.aquaculture_aerator
    name: Aerator
    icon: mdi:air-purifier
  
  - entity: switch.aquaculture_water_pump
    name: Water Pump
    icon: mdi:pump
  
  - entity: switch.aquaculture_circulation
    name: Circulation
    icon: mdi:water-pump
  
  - entity: switch.aquaculture_feeder
    name: Feeder
    icon: mdi:fish-food
```

### Option C: Beautiful Grid Card

```yaml
type: grid
columns: 2
square: false
title: Aquaculture Pond
entities:
  - entity: sensor.aquaculture_temperature
    type: custom:bar-card
    entity_row: true
    min: 0
    max: 40
    unit_of_measurement: °C
  
  - entity: sensor.aquaculture_ph
    type: custom:bar-card
    entity_row: true
    min: 0
    max: 14
    unit_of_measurement: pH
  
  - entity: sensor.aquaculture_do
    type: custom:bar-card
    entity_row: true
    min: 0
    max: 12
    unit_of_measurement: mg/L
  
  - entity: sensor.aquaculture_water_level
    type: custom:bar-card
    entity_row: true
    min: 0
    max: 100
    unit_of_measurement: "%"
```

## MQTT Topics Reference

### Publish FROM ESP32 TO Home Assistant
```
smartfarm/aquaculture/sensor/water_temp       → Water temperature
smartfarm/aquaculture/sensor/ph               → pH value
smartfarm/aquaculture/sensor/ph_trend         → pH trend
smartfarm/aquaculture/sensor/do               → DO value
smartfarm/aquaculture/sensor/water_level      → Water level %
smartfarm/aquaculture/sensor/turbidity        → Turbidity
smartfarm/aquaculture/sensor/air_temp         → Air temperature
smartfarm/aquaculture/sensor/humidity         → Humidity
smartfarm/aquaculture/sensor/light            → Light
smartfarm/aquaculture/sensor/co2              → CO2 (disabled by default in V2)
smartfarm/aquaculture/sensor/aerator_current  → Aerator current
smartfarm/aquaculture/sensor/pump_current     → Pump current
smartfarm/aquaculture/controller/state        → Controller mode/state
smartfarm/aquaculture/controller/status       → Controller status text
smartfarm/aquaculture/safety/state            → Safety state text
smartfarm/aquaculture/alarm/text              → Alarm text

smartfarm/aquaculture/output/aerator       → Aerator ON/OFF
smartfarm/aquaculture/output/aerator_1     → Aerator 1 ON/OFF (same hardware as aerator)
smartfarm/aquaculture/output/aerator_2     → Aerator 2 ON/OFF (unavailable if hardware absent)
smartfarm/aquaculture/output/pump          → Pump ON/OFF
smartfarm/aquaculture/output/circulation   → Circulation ON/OFF
smartfarm/aquaculture/output/feeder        → Feeder ON/OFF
smartfarm/aquaculture/output/alarm         → Alarm output ON/OFF

smartfarm/aquaculture/config/species/state  (MQTT_TOPIC_SPECIES_STATE)
smartfarm/aquaculture/config/mode/state     (MQTT_TOPIC_MODE_STATE)
smartfarm/aquaculture/controller/state      (MQTT_TOPIC_STATE)
```

### Subscribe FROM Home Assistant TO ESP32
```
smartfarm/aquaculture/control/aerator/set        ← Control aerator
smartfarm/aquaculture/control/aerator_1/set      ← Control aerator 1
smartfarm/aquaculture/control/aerator_2/set      ← Control aerator 2 (ignored if hardware absent)
smartfarm/aquaculture/control/pump/set           ← Control pump
smartfarm/aquaculture/control/circulation/set    ← Control circulation
smartfarm/aquaculture/control/feeder/set         ← Control feeder
smartfarm/aquaculture/control/alarm/set          ← Control alarm output (ignored if hardware absent)

smartfarm/aquaculture/config/species/set         ← Change species
smartfarm/aquaculture/config/mode/set            ← Change mode
```

## Testing

### Check MQTT Connection
```bash
mosquitto_sub -h 192.168.1.100 -u user -P pass -v -t 'smartfarm/aquaculture/#'
```

You should see:
```
smartfarm/aquaculture/sensor/temperature 27.5
smartfarm/aquaculture/sensor/ph 7.8
smartfarm/aquaculture/sensor/do 6.2
smartfarm/aquaculture/sensor/level 85.3
```

### Send Test Command
```bash
mosquitto_pub -h 192.168.1.100 -u user -P pass -t 'smartfarm/aquaculture/control/aerator/set' -m 'ON'
```

## Troubleshooting

### Entities not appearing?
1. Check MQTT connection: `mqtt info` in Home Assistant console
2. Verify broker IP in `app_config.h`
3. Check ESP32 serial monitor for MQTT connection logs
4. Restart Home Assistant: Settings → System → Restart

### Still seeing old V1 entities?

V2 uses device metadata and unique IDs prefixed with `aquaculture_v2_...`.
If Home Assistant still shows old V1 retained entities, only clear Aquaculture discovery topics:

```bash
for topic in \
  homeassistant/sensor/aquaculture_water_temp/config \
  homeassistant/sensor/aquaculture_ph/config \
  homeassistant/sensor/aquaculture_ph_trend/config \
  homeassistant/sensor/aquaculture_do/config \
  homeassistant/sensor/aquaculture_water_level/config \
  homeassistant/sensor/aquaculture_turbidity/config \
  homeassistant/sensor/aquaculture_air_temp/config \
  homeassistant/sensor/aquaculture_humidity/config \
  homeassistant/sensor/aquaculture_light/config \
  homeassistant/sensor/aquaculture_co2/config \
  homeassistant/sensor/aquaculture_aerator_current/config \
  homeassistant/sensor/aquaculture_pump_current/config \
  homeassistant/sensor/aquaculture_controller_state/config \
  homeassistant/sensor/aquaculture_controller_status/config \
  homeassistant/sensor/aquaculture_safety_state/config \
  homeassistant/sensor/aquaculture_alarm_text/config \
  homeassistant/binary_sensor/aquaculture_alarm_active/config \
  homeassistant/binary_sensor/aquaculture_safety_active/config \
  homeassistant/binary_sensor/aquaculture_emergency_active/config \
  homeassistant/switch/aquaculture_pump/config \
  homeassistant/switch/aquaculture_aerator/config \
  homeassistant/switch/aquaculture_aerator_1/config \
  homeassistant/switch/aquaculture_aerator_2/config \
  homeassistant/switch/aquaculture_circulation/config \
  homeassistant/switch/aquaculture_feeder/config \
  homeassistant/switch/aquaculture_alarm_output/config \
  homeassistant/select/aquaculture_mode/config \
  homeassistant/select/aquaculture_species/config; do
  mosquitto_pub -h <broker> -u <user> -P <password> -t "$topic" -r -n
done
```

Then reboot ESP32 so discovery for **Aquaculture Controller V2** is republished.

### Slow updates?
1. Reduce `MQTT_PUBLISH_INTERVAL` in `include/app_config.h` (milliseconds)
2. Check WiFi signal strength
3. Monitor MQTT broker CPU load

### Commands not working?
1. Verify ESP32 is in MANUAL or SCHEDULE mode (not SAFE)
2. Check MQTT subscriptions are active
3. Look at serial monitor for incoming messages
4. If `BENCH_TEST_MODE=true`, all relay ON commands are intentionally suppressed and state stays OFF

## Advanced: Automations

### Auto-enable aerator when DO drops
```yaml
automation:
  - id: low_do_aerator
    alias: "Low DO - Enable Aerator"
    trigger:
      platform: numeric_state
      entity_id: sensor.aquaculture_do
      below: 5.0
    action:
      service: switch.turn_on
      entity_id: switch.aquaculture_aerator
```

### Alert when temperature too high
```yaml
automation:
  - id: high_temp_alert
    alias: "High Temperature Alert"
    trigger:
      platform: numeric_state
      entity_id: sensor.aquaculture_temperature
      above: 32
    action:
      service: notify.send_notification
      data:
        message: "Aquaculture temperature too high!"
```

## Next Steps

1. ✅ Upload firmware with MQTT discovery enabled
2. ✅ Create dashboard
3. ✅ Test sensor readings
4. ✅ Test relay control
5. ✅ Set up automations
6. ✅ Deploy to production
