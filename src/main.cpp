#include "config.h"
#include <TestFunctions.h>
#include <Arduino.h>

#include <HomeAssistant.h>
#include <Ina219Monitor.h>
#include <MosfetControl.h>
#include <ExperimentController.h>
#include <WifiConnection.h>

MosfetControl mosfetControl(CHARGE_PIN, LOAD_PIN);
WifiConnection wifiConnection;
Ina219Monitor ina219Monitor(INA219_CALC_ADDRESS(INA_ADDR0, INA_ADDR1));
HomeAssistant homeAssistant;
ExperimentController experimentController(mosfetControl, homeAssistant);

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
        Serial.println("Measurement command received: off");
        homeAssistant.setMeasurementCommandStatus(false);
    } else if (command == HomeAssistant::MeasuringCommand) {
        Serial.println("Measurement command received: measuring");
        homeAssistant.setMeasurementCommandStatus(true);
    }
    experimentController.handleCommand(millis(), command);
    experimentController.update(millis());

    static uint32_t lastSample = 0;
    static bool hasReadings = false;
    static Ina219Readings readings = {};
    const uint32_t now = millis();
    if (!hasReadings || now - lastSample >= EXPERIMENT_SAMPLE_INTERVAL_MS) {
        lastSample = now;
        readings = ina219Monitor.read();
        hasReadings = true;
        experimentController.processReadings(now, readings);
    }

    static uint32_t lastPublish = 0;
    if (hasReadings && now - lastPublish >= MQTT_PUBLISH_INTERVAL_MS) {
        lastPublish = now;
        printIna219Readings(readings);
        experimentController.printStatus(now);
        homeAssistant.publishReadings(readings);
        homeAssistant.publishExperimentState();
        homeAssistant.publishMeasurementCommandStatus();
        if (experimentController.hasCoulombData()) {
            homeAssistant.publishDischargedAh(experimentController.dischargedAh());
        }
        if (experimentController.hasStateOfCharge()) {
            homeAssistant.publishStateOfCharge(experimentController.stateOfChargePercent());
        }
    }

    // testMosfetControl(mosfetControl);
    // testIna219Readout(ina219Monitor);
}