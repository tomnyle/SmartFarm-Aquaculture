#include "schedule_mode.h"
#include <ArduinoJson.h>
#include <string.h>
#include "../relays/relay_names.h"

void ScheduleMode::configureDefaults(ScheduleState& schedule) const {
  schedule.enabled = true;
  schedule.entryCount = 3;
  ScheduleEntry defaults[3] = {{"feeder", 8, 0, 10, 0}, {"feeder", 12, 0, 10, 0}, {"feeder", 17, 0, 10, 0}};
  for (uint8_t i = 0; i < schedule.entryCount; ++i) {
    schedule.entries[i] = defaults[i];
  }
}

bool ScheduleMode::updateFromJson(const String& json, ScheduleState& schedule) const {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
  JsonArray entries = doc["entries"].as<JsonArray>();
  if (entries.isNull()) return false;
  schedule.enabled = doc["enabled"] | true;
  schedule.entryCount = 0;
  for (JsonObject entry : entries) {
    if (schedule.entryCount >= constants::MAX_SCHEDULE_ENTRIES) break;
    ScheduleEntry& target = schedule.entries[schedule.entryCount++];
    strlcpy(target.relayName, entry["relay"] | relay_names::FEEDER, sizeof(target.relayName));
    target.hour = entry["hour"] | 0;
    target.minute = entry["minute"] | 0;
    target.durationSeconds = entry["duration"] | 10;
    target.lastRunDay = 0;
  }
  return true;
}

void ScheduleMode::tick(ScheduleState& schedule, RelayManager& relayManager) const {
  if (!schedule.enabled) return;
  time_t now = time(nullptr);
  if (now <= 0) return;
  struct tm localTime;
  localtime_r(&now, &localTime);
  const uint32_t dayKey = static_cast<uint32_t>(localTime.tm_yday + (localTime.tm_year * 366));
  for (uint8_t i = 0; i < schedule.entryCount; ++i) {
    ScheduleEntry& entry = schedule.entries[i];
    if (entry.lastRunDay == dayKey) continue;
    if (entry.hour == localTime.tm_hour && entry.minute == localTime.tm_min) {
      relayManager.setRelay(entry.relayName, true);
      delay(entry.durationSeconds * 1000UL);
      relayManager.setRelay(entry.relayName, false);
      entry.lastRunDay = dayKey;
    }
  }
}
