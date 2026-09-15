#include "schedule_mode.h"
#include <ArduinoJson.h>
#include <string.h>
#include "../relays/relay_names.h"

void ScheduleMode::configureDefaults(ScheduleState& schedule) const {
  schedule.enabled = true;
  schedule.entryCount = 3;
  schedule.activeRun = false;
  schedule.activeRelayName[0] = '\0';
  schedule.activeStartedMs = 0;
  schedule.activeDurationMs = 0;
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
  ScheduleState next = schedule;
  next.enabled = doc["enabled"] | true;
  next.entryCount = 0;
  next.activeRun = false;
  next.activeRelayName[0] = '\0';
  next.activeStartedMs = 0;
  next.activeDurationMs = 0;
  for (JsonObject entry : entries) {
    if (next.entryCount >= constants::MAX_SCHEDULE_ENTRIES) return false;
    const uint16_t durationSeconds = entry["duration"] | 0;
    if (durationSeconds == 0) return false;
    ScheduleEntry& target = next.entries[next.entryCount++];
    strlcpy(target.relayName, entry["relay"] | relay_names::FEEDER, sizeof(target.relayName));
    target.hour = entry["hour"] | 0;
    target.minute = entry["minute"] | 0;
    target.durationSeconds = durationSeconds;
    target.lastRunDay = 0;
  }
  schedule = next;
  return true;
}

void ScheduleMode::tick(ScheduleState& schedule, RelayManager& relayManager) const {
  if (!schedule.enabled) return;
  const uint32_t nowMs = millis();
  if (schedule.activeRun && static_cast<uint32_t>(nowMs - schedule.activeStartedMs) >= schedule.activeDurationMs) {
    relayManager.setRelay(schedule.activeRelayName, false);
    schedule.activeRun = false;
    schedule.activeRelayName[0] = '\0';
    schedule.activeStartedMs = 0;
    schedule.activeDurationMs = 0;
  }
  if (schedule.activeRun) return;
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
      strlcpy(schedule.activeRelayName, entry.relayName, sizeof(schedule.activeRelayName));
      schedule.activeRun = true;
      schedule.activeStartedMs = nowMs;
      schedule.activeDurationMs = entry.durationSeconds * 1000UL;
      entry.lastRunDay = dayKey;
    }
  }
}
