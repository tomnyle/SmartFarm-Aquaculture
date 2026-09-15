# Hardware Wiring

This page documents the current pin map used by firmware (`include/pins.h`).

## ESP32 Pin Mapping

### Sensor inputs

- `GPIO4`  -> DS18B20 data (1-Wire)
- `GPIO15` -> DHT22 data
- `GPIO34` -> pH analog input
- `GPIO35` -> Turbidity analog input
- `GPIO36` -> Dissolved oxygen analog input
- `GPIO39` -> CO2 analog input
- `GPIO21` -> I2C SDA (BH1750)
- `GPIO22` -> I2C SCL (BH1750)

### Relay outputs

- `GPIO13` -> Pump relay
- `GPIO12` -> Aerator relay
- `GPIO14` -> Circulation relay
- `GPIO27` -> Feeder relay

Optional:

- `GPIO2` -> Status LED

## Sensor connection notes

### DS18B20 (water temperature)

- VCC -> 3.3V (or module-compatible supply)
- GND -> GND
- DATA -> GPIO4
- Add a 4.7k pull-up resistor between DATA and VCC

### DHT22 (air temperature/humidity)

- VCC -> 3.3V
- GND -> GND
- DATA -> GPIO15

### Analog sensors (pH, DO, CO2, turbidity)

- Connect sensor analog output to mapped ADC pin
- ESP32 ADC range is 0-3.3V
- **Do not feed 5V analog output directly** into ESP32 ADC pins
- Use signal conditioning/voltage divider if sensor board outputs above 3.3V

Current firmware scaling assumptions (`src/main.cpp`):

- pH: `(ADC / 4095) * 14`
- DO: `(ADC / 4095) * 20`
- CO2: `(ADC / 4095) * 10`
- Turbidity: raw ADC count

### BH1750 (I2C light sensor)

- VCC -> 3.3V (check module support)
- GND -> GND
- SDA -> GPIO21
- SCL -> GPIO22

Important:

- BH1750 is optional; firmware continues if not detected
- Keep I2C wiring short and stable
- Use pull-up resistors if your module board does not include them

## Relay module wiring

- ESP32 GPIO -> relay IN pin
- Relay module VCC/GND -> dedicated relay power rail (as required by module)
- Keep relay power and logic grounds common
- Wire high-voltage loads only through relay contacts (COM/NO/NC)

## Power and safety notes

- Use a stable PSU sized for ESP32 + sensors + relay coils
- Avoid powering high-current relays directly from ESP32 3.3V pin
- Keep sensor ground reference clean to reduce analog noise
- Add fuses/breakers for pump and mains-powered equipment
- Isolate mains wiring and follow local electrical safety rules
