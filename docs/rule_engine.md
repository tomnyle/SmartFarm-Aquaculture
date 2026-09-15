# Rule Engine

This document describes the current AUTO logic in `src/main.cpp` and species profiles in `include/species_rules.h`.

## Supported species profiles

The active species is read from `smartfarm/aquaculture/config/species/state` and controlled via `.../set`.

Profiles currently implemented by rule lookup:

- `Cá Chép`
- `Rô Phi` (default)
- `Cá Tra`
- `Tôm Thẻ`

If an unknown species string is selected, firmware falls back to **Rô Phi** thresholds.

Home Assistant currently exposes more selectable labels (`Koi`, `Cá Trắm`, `Cá Lóc`, `Tôm Sú`, `Tilapia`) than the rule mapper supports; those values will also fall back to **Rô Phi** until mapping is added.

## Threshold table

| Species | Temp min/max (°C) | Temp critical low/high (°C) | pH min/max | DO min / critical (mg/L) | CO2 max | Turbidity max |
|---|---:|---:|---:|---:|---:|---:|
| Cá Chép | 20.0 / 28.0 | 15.0 / 30.0 | 7.0 / 8.5 | 5.5 / 4.0 | 10.0 | 80.0 |
| Rô Phi | 22.0 / 30.0 | 18.0 / 33.0 | 6.5 / 8.5 | 4.5 / 3.5 | 12.0 | 100.0 |
| Cá Tra | 24.0 / 30.0 | 20.0 / 32.0 | 6.5 / 8.0 | 5.0 / 4.0 | 10.0 | 100.0 |
| Tôm Thẻ | 26.0 / 32.0 | 24.0 / 33.0 | 7.5 / 8.8 | 5.5 / 4.5 | 8.0 | 60.0 |

## Mode behavior

### AUTO

- Rule engine runs every `RULE_ENGINE_INTERVAL` (5s)
- Sensor values are validated/clamped for obvious invalid ranges
- Outputs are toggled by species thresholds and safety conditions

### MANUAL

- MQTT control topics can directly toggle outputs
- Current implementation sets mode to `MANUAL` when pump command is received
- AUTO rule loop does not run unless mode is switched back to `AUTO`

### SCHEDULE

- Mode is exposed via MQTT/Home Assistant selector
- No dedicated schedule execution logic is currently implemented in `src/main.cpp`

### SAFE

- Mode is exposed via MQTT/Home Assistant selector
- No dedicated SAFE fallback handler is currently implemented in `src/main.cpp`

## Trigger logic (AUTO)

Conditions and resulting outputs:

- `DO <= do_critical` -> `aerator = ON`, `pump = ON`
- `water_temp >= temp_critical_high` -> `circulation = ON`, `pump = ON`
- `DO < do_min` -> `aerator = ON`
- `water_temp > temp_max` -> `circulation = ON`
- `CO2 > co2_max` -> `pump = ON`, `aerator = ON`
- `DO < do_min + 0.5` -> `circulation = ON`

Current feeder behavior:

- Feeder is always set to OFF in AUTO logic (both stable and alert branches)

## Hysteresis and safety timing

The firmware uses minimum change intervals to reduce rapid relay toggling:

- Pump change gate: 30s (`PUMP_MIN_OFF_TIME`)
- Aerator change gate: 5s
- Circulation change gate: 5s
- Feeder change gate: 60s

Rule execution and telemetry timing:

- Sensor read interval: 5s
- Rule engine interval: 5s (AUTO only)
- MQTT publish interval: 10s

## MQTT entities affected

AUTO/MANUAL updates are visible via these retained state topics:

- `smartfarm/aquaculture/output/pump`
- `smartfarm/aquaculture/output/aerator`
- `smartfarm/aquaculture/output/circulation`
- `smartfarm/aquaculture/output/feeder`
- `smartfarm/aquaculture/config/mode/state`
- `smartfarm/aquaculture/config/species/state`
