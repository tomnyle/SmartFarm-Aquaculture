# Operating Modes

## AUTO
Evaluates sensor conditions against the active species profile and controls aerator, pump, and circulation automatically.

## MANUAL
Accepts relay commands from Home Assistant without AUTO intervention.

## SCHEDULE
Runs scheduled relay actions from the configured schedule JSON, with default feeder times at 08:00, 12:00, and 17:00.

## SAFE
Triggered by critical sensor conditions. Defaults to aerator ON and other relays OFF.
