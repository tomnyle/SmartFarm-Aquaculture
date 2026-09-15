#include <WiFi.h>
#include <time.h>
#include "wifi_manager.h"
#include "../config/app_config.h"
#include "../../include/constants.h"

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(app_config::WIFI_SSID, app_config::WIFI_PASSWORD);
}

void WiFiManager::ensureConnected() {
  if (WiFi.status() == WL_CONNECTED) return;
  begin();
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(constants::WIFI_RETRY_DELAY_MS);
  }
  if (WiFi.status() == WL_CONNECTED) {
    configTime(app_config::GMT_OFFSET_SECONDS, app_config::DAYLIGHT_OFFSET_SECONDS,
               app_config::NTP_SERVER_1, app_config::NTP_SERVER_2);
  }
}
