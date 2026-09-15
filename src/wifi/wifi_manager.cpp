#include "wifi_manager.h"
#include "../config/app_config.h"
#include "../utils/logger.h"

void WiFiManager::connect() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t started_at = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - started_at) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Logger::info("WiFi connected / Da ket noi WiFi");
    } else {
        Logger::warn("WiFi timeout / Het thoi gian ket noi WiFi");
    }
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}
