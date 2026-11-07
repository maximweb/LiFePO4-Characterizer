#include "config.h"
#include <TestFunctions.h>
#include <Arduino.h>

#include <HomeAssistant.h>
#include <Ina219Monitor.h>
#include <MosfetControl.h>
#include <WifiConnection.h>

MosfetControl mosfetControl(CHARGE_PIN, LOAD_PIN);
WifiConnection wifiConnection;
Ina219Monitor ina219Monitor(INA219_CALC_ADDRESS(INA_ADDR0, INA_ADDR1));
HomeAssistant homeAssistant;

/**
 * Setup function
 */
void setup() {
    // Setup MOSFET control pins, do this at the very beginning
    // to ensure they are off before any other setup occurs
    mosfetControl.begin();

    Serial.begin(115200);
    // TODO: Unsire if needed so delay of PIO "Upload and monitor"
    // does not cut off the initial serial output
    delay(1000);
    Serial.println("Starting setup...");

    // Initialize WiFi connection
    wifiConnection.begin(WIFI_SSID, WIFI_PASSWORD);

    // INA219 initialization
    Serial.println("Initializing INA219...");
    if (!ina219Monitor.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1) {
            delay(10);
        }
    }

    homeAssistant.begin(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT, MQTT_USERNAME, MQTT_PASSWORD);

    Serial.println("Setup complete");
}

/**
 * Main loop function
 */
void loop() {
    homeAssistant.loop();

    const int8_t command = homeAssistant.takeCommand();
    if (command == HomeAssistant::OffCommand) {
        mosfetControl.setState(MosfetState::Off);
    } else if (command == HomeAssistant::MeasuringCommand) {
        Serial.println("Measurement started");
    }

    static uint32_t lastPublish = 0;
    if (millis() - lastPublish >= 2000) {
        lastPublish = millis();
        homeAssistant.publishReadings(ina219Monitor.read());
    }

    // testMosfetControl(mosfetControl);
    // testIna219Readout(ina219Monitor);
}