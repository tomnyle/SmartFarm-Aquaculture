# Troubleshooting

## No Home Assistant entities
- Verify MQTT broker settings in `src/config/app_config.h`
- Confirm discovery is enabled in Home Assistant
- Check that the ESP32 publishes `status/availability`

## No relay response
- Confirm the relay pin map in `src/config/relay_config.h`
- Test with MANUAL mode from Home Assistant

## Unexpected SAFE mode
- Inspect `status/error`
- Review `conditions/*` topics for the critical input
