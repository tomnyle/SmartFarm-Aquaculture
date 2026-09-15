#ifndef SMARTFARM_WIFI_MANAGER_H
#define SMARTFARM_WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
    void connect();
    bool isConnected() const;
};

#endif
