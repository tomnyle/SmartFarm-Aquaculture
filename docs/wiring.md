# Wiring Guide

## GPIO map

| Function | Pin | Notes |
|---|---:|---|
| DS18B20 water temp | GPIO4 | 1-Wire, add 4.7k pull-up |
| DHT22 air temp/humidity | GPIO15 | Optional ambient sensor |
| I2C SDA | GPIO21 | BH1750 / future ADS1115 |
| I2C SCL | GPIO22 | BH1750 / future ADS1115 |
| pH analog | GPIO34 | ADC1 input |
| Turbidity analog | GPIO35 | ADC1 input |
| DO analog | GPIO36 | ADC1 input |
| CO2 analog | GPIO39 | ADC1 input |
| Water level analog | GPIO32 | ADC1 input |
| Aerator current analog | GPIO33 | ADC1 input |
| Pump current analog | GPIO25 | ADC2 input; consider external ADC for Wi-Fi-heavy installs |
| Pump relay | GPIO13 | Output |
| Aerator 1 relay | GPIO12 | Output |
| Circulation relay | GPIO14 | Output |
| Feeder relay | GPIO27 | Output |
| Aerator 2 relay | GPIO26 | Output |
| Alarm relay/buzzer | GPIO23 | Output |
| Status LED | GPIO2 | Optional |

## Relay / contactor guidance

- Use relay boards or contactors rated for the actual motor current.
- Large aerators and pumps should usually switch through a **contactor**, not directly from a small relay board.
- If the feeder motor is inductive, add the proper flyback / snubber protection.

## Example output wiring

- **Aerator 1 / 2** -> relay or contactor coils
- **Pump** -> relay/contactors with overload protection
- **Feeder** -> motor driver / relay suitable for the feeder motor
- **Alarm** -> buzzer, siren, or warning beacon relay

## Analog sensor caveats

- ESP32 ADC readings are noisy; use shielding, stable grounding, and sensor-side filtering.
- Keep pH and DO signal wiring away from AC motor wiring.
- ADC2 channels can be less convenient when Wi-Fi is active; for production current monitoring, an external ADC is often cleaner.

## Electrical safety checklist

- Use RCCB/GFCI on pond mains circuits.
- Add surge protection and lightning protection where appropriate.
- Mount electronics in waterproof, ventilated enclosures.
- Separate low-voltage and mains wiring paths.
- Label disconnects for pump, aerator, and feeder circuits.
- Test emergency power-loss behavior before live use.
