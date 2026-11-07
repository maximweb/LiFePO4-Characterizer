#pragma once

class WifiConnection {
  public:
    /** Connects to the configured WiFi network and waits until connected. */
    void begin(const char* ssid, const char* password);
};