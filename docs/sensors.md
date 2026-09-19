# Sensor Guide

## DO is the primary sensor

For a practical family pond controller, **DO is the main safety input**. The controller should react to DO drops locally without waiting for Home Assistant.

### V1 DO policy
- DO warning -> Aerator 1 ON + warning event
- DO low -> Aerator 1 ON + alarm
- DO critical -> SAFE behavior + both aerators ON + feeder lock
- DO emergency -> EMERGENCY behavior + both aerators ON + alarm
- DO invalid / stale -> sensor fault, safe aeration fallback

## pH sensor

### V1 use
- Show current pH
- Warn when pH is outside the configured/species range
- Warn when pH changes too quickly (`PH_RATE_LIMIT`)
- **Do not auto-dose chemicals** in V1

### Calibration
- Perform at least a two-point calibration with fresh buffer solutions.
- Re-check calibration after cleaning or probe replacement.
- Record any offset/slope change in maintenance notes.

## DS18B20 water temperature

### V1 use
- Feed into safe/critical temperature checks
- Lock feeder and force aeration when water temperature is critical
- Use the waterproof probe variant for pond deployment

## Water level

### V1 use
- Detect low and critical water levels
- Auto-request refill pump in AUTO/SCHEDULE when below low threshold
- Stop pump and raise a device fault if runtime exceeds `PUMP_MAX_RUN_TIME_MS` without recovery
- Lock feeder when level is critical

## Current monitoring hooks

### Aerator current
Used as a verification hook:
- Output says ON, but current is below `CURRENT_MIN_RUNNING_A` -> possible relay / motor / power fault
- Current above `CURRENT_MAX_RUNNING_A` -> possible overload

### Pump current
Used similarly for pump fault detection.

> Note: V1 treats these as hooks. Final current calibration depends on the installed CT/Hall sensor and analog front-end.

## V2 sensors

These are intentionally deferred until V1 is stable:
- ORP
- NH3/NH4 online sensing
- NO2 online sensing

For V1, NH3/NH4 and NO2 can be tracked manually with test kits and entered into Home Assistant notes/logs.
