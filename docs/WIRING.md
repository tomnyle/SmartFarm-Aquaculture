# WIRING

## Core Wiring

- DS18B20 -> GPIO4 (1-Wire, pull-up 4.7k)
- ADS1115 #1 -> SDA GPIO21, SCL GPIO22
- pH -> ADS1115 #1 A0
- DO -> ADS1115 #1 A1
- Water Level -> GPIO23

## Relay Mapping

1. Aerator -> GPIO12
2. Pump -> GPIO13
3. Circulation -> GPIO14
4. Feeder -> GPIO27
5. Valve -> GPIO26
6. Light -> GPIO25
7. Spare 1 -> GPIO33
8. Spare 2 -> GPIO32

## Expansion Wiring

- ADS1115 #2 address `0x49` dành cho EC/TDS, ORP, Turbidity hoặc CO₂ trong các phase sau.
