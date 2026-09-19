# SmartFarm Aquaculture Controller V1

ESP32-based SmartAquaculture controller for a family pond around **200 m²**, designed to keep the pond safe even when Wi-Fi, MQTT, or Home Assistant are unavailable.

## V1 goals

- Put **dissolved oxygen (DO)** at the center of control decisions.
- Keep a **local fail-safe rule engine** on the ESP32.
- Support **AUTO / MANUAL / SCHEDULE / SAFE / EMERGENCY** operating modes.
- Detect **sensor faults / stale data**, enforce **water-level protection**, and lock feeding when water quality is unsafe.
- Publish structured state to **MQTT** and expose entities through **Home Assistant discovery**.

## 5-layer architecture

1. **Sensors** – DO, pH, water temperature, water level, turbidity, CO2, air temp/humidity, light, current hooks.
2. **Control** – aerator 1, aerator 2, pump, circulation, feeder, alarm.
3. **Safety** – stale/invalid sensor detection, DO-first protection, pump timeout, feeder lock, current-fault hooks.
4. **MQTT / Home Assistant** – telemetry, commands, discovery, alarm events, heartbeat.
5. **Data logging** – MQTT retention, Home Assistant recorder, optional InfluxDB / Grafana.

## V1 feature list

- Local DO-driven aeration policy with warning / low / critical / emergency thresholds.
- Local SAFE / EMERGENCY override even if MQTT is down.
- Water-level low / critical handling with pump timeout protection.
- pH warning by **range** and **rate-of-change**.
- Temperature-critical feeder lock and forced aeration.
- Manual mode timeout and safety interlock protection.
- Home Assistant discovery for key sensors, outputs, mode, and alarm state.
- Alarm, sensor-fault, and device-fault MQTT events.

## V2 roadmap

Planned upgrades after V1 is stable in the pond:

- ORP
- NH3/NH4 and NO2 online sensing
- Camera integration via Home Assistant
- InfluxDB / Grafana dashboards
- Feed history, biomass estimate, FCR tracking
- Split architecture: Sensor Node + Control Node

## Quick start

1. **Clone the repository**
   ```bash
   git clone https://github.com/tomnyle/SmartFarm-Aquaculture.git
   cd SmartFarm-Aquaculture
   ```
2. **Edit `/home/runner/work/SmartFarm-Aquaculture/SmartFarm-Aquaculture/include/app_config.h`**
   - Set `WIFI_SSID` / `WIFI_PASSWORD`
   - Set `MQTT_BROKER` / `MQTT_USER` / `MQTT_PASSWORD`
   - Review pond-safe thresholds before deployment
3. **Build firmware**
   ```bash
   platformio run -e esp32dev
   ```
4. **Upload firmware**
   ```bash
   platformio run -e esp32dev -t upload
   ```
5. **Open the serial monitor**
   ```bash
   platformio device monitor -b 115200
   ```
6. **Enable MQTT discovery in Home Assistant**, then review the dashboard example in `docs/home-assistant-dashboard.yaml`.

## Safe defaults

- **DO sensor fault or stale data** → SAFE behavior, feeder lock, alarm, at least one aerator ON.
- **DO critical / emergency** → both aerators ON, feeder locked, alarm event published.
- **Water level critical** → pump protected, feeder locked, alarm ON.
- **Temperature critical** → feeder locked, forced aeration.
- **Feeder runtime limit** and **pump runtime timeout** are enforced locally.

## Repository layout

- `src/main.cpp` – active V1 firmware loop, rule engine, MQTT, Home Assistant discovery.
- `include/app_config.h` – credentials placeholders, MQTT topics, safety thresholds, runtime constants.
- `include/pins.h` – GPIO map for sensors, relays, alarm, and current hooks.
- `include/aquaculture_logic.h` – control mode, safety level, sensor/output state structures.
- `include/species_rules.h` – basic species-specific advisory limits.
- `docs/architecture.md` – V1 and future 2-node architecture.
- `docs/sensors.md` – sensor roles, calibration, and V1/V2 sensing strategy.
- `docs/wiring.md` – wiring map and electrical safety guidance.
- `docs/mqtt.md` – MQTT topic specification and payload examples.
- `docs/home-assistant-dashboard.yaml` – example Lovelace dashboard.
- `docs/operation-checklist.md` – daily/weekly/monthly and emergency operations.

## Electrical safety warnings

- **Do not switch aerators, pumps, or feeders directly from the ESP32 GPIO pins.** Use proper relay modules, contactors, drivers, or SSRs rated for the real load.
- Protect outdoor pond circuits with **RCCB/GFCI**, surge protection, waterproof enclosures, and correct earthing.
- Keep low-voltage logic wiring separated from mains wiring.
- Add manual disconnects and test emergency shutdown procedures before stocking the pond.

## Documentation index

See the `/docs` directory for architecture, wiring, MQTT, sensor deployment, dashboard, and operating checklists.
