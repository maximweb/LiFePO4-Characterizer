#include "config.h"
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Adafruit_INA219.h>
#include <Wire.h>

Adafruit_INA219 ina219(INA219_CALC_ADDRESS(INA_ADDR0, INA_ADDR1));

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/**
 * @brief Scan the I2C bus for devices and print their addresses
 */
void scanI2C() {
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("I2C device at 0x%02X\n", addr);
            found++;
        }
    }
    Serial.printf("I2C scan done, %d device(s)\n", found);
}

/**
 * @brief Enum for the mutually exclusive MOSFET states
 */
enum MosfetState { MOSFET_OFF = 0, MOSFET_CHARGE = 1, MOSFET_LOAD = 2 };

/**
 * @brief Set mutually exclusive MOSFET state
 * @note This function ensures that only one MOSFET is active at a time, or both
 * are off.
 * @param state The desired MOSFET state
 * @return true if the MOSFETs were successfully switched to the desired state,
 * false otherwise
 */
bool setMosfetState(MosfetState state) {
    bool stateConfirmed = false;

    switch (state) {
    case MOSFET_OFF:
        digitalWrite(CHARGE_PIN, LOW);
        digitalWrite(LOAD_PIN, LOW);
        delay(10); // Small delay to allow the pins to settle

        stateConfirmed = digitalRead(CHARGE_PIN) == LOW && digitalRead(LOAD_PIN) == LOW;
        break;

    case MOSFET_CHARGE:
        digitalWrite(LOAD_PIN, LOW);
        delay(10); // Small delay to allow the pins to settle

        if (digitalRead(LOAD_PIN) == LOW) {
            digitalWrite(CHARGE_PIN, HIGH);
            delay(10); // Small delay to allow the pins to settle

            stateConfirmed = digitalRead(CHARGE_PIN) == HIGH && digitalRead(LOAD_PIN) == LOW;
        }
        break;

    case MOSFET_LOAD:
        digitalWrite(CHARGE_PIN, LOW);
        delay(10); // Small delay to allow the pins to settle

        if (digitalRead(CHARGE_PIN) == LOW) {
            digitalWrite(LOAD_PIN, HIGH);
            delay(10); // Small delay to allow the pins to settle

            stateConfirmed = digitalRead(CHARGE_PIN) == LOW && digitalRead(LOAD_PIN) == HIGH;
        }
        break;

    default:
        break;
    }

    if (stateConfirmed) {
        return true;
    }

    // Safeguard: turn off both MOSFETs if the desired state could not be
    // confirmed
    digitalWrite(CHARGE_PIN, LOW);
    digitalWrite(LOAD_PIN, LOW);
    delay(10); // Small delay to allow the pins to settle
    return false;
}

/**
 * Setup function
 */
void setup() {
    // Initialize Mosfet pins first, to ensure they are off
    digitalWrite(CHARGE_PIN, LOW);
    digitalWrite(LOAD_PIN, LOW);
    pinMode(CHARGE_PIN, OUTPUT);
    pinMode(LOAD_PIN, OUTPUT);
    digitalWrite(CHARGE_PIN, LOW);
    digitalWrite(LOAD_PIN, LOW);

    delay(1000);

    Serial.begin(115200);
    Serial.println("Starting setup...");

    // Wifi connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // MQTT client setup
    mqttClient.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);

    // INA219 initialization
    Wire.begin();
    scanI2C();
    Serial.println("Initializing INA219...");
    if (!ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1) {
            delay(10);
        }
    }

    Serial.println("Setup complete");
}

/**
 * Main loop function
 */
void loop() {
    // TODO: Test MOSFET control
    Serial.println("Testing MOSFET control...");
    Serial.print("Switching to MOSFET_OFF: ");
    Serial.println(setMosfetState(MOSFET_OFF) ? "Success" : "Failure");
    delay(1000);

    Serial.print("Switching to MOSFET_CHARGE: ");
    Serial.println(setMosfetState(MOSFET_CHARGE) ? "Success" : "Failure");
    delay(1000);

    Serial.print("Switching to MOSFET_LOAD: ");
    Serial.println(setMosfetState(MOSFET_LOAD) ? "Success" : "Failure");
    delay(1000);

    // TODO: Test INA219 readout
    float shuntvoltage = 0;
    float busvoltage = 0;
    float current_mA = 0;
    float loadvoltage = 0;
    float power_mW = 0;

    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);

    Serial.print("Bus Voltage:   ");
    Serial.print(busvoltage);
    Serial.println(" V");
    Serial.print("Shunt Voltage: ");
    Serial.print(shuntvoltage);
    Serial.println(" mV");
    Serial.print("Load Voltage:  ");
    Serial.print(loadvoltage);
    Serial.println(" V");
    Serial.print("Current:       ");
    Serial.print(current_mA);
    Serial.println(" mA");
    Serial.print("Power:         ");
    Serial.print(power_mW);
    Serial.println(" mW");
    Serial.println("");

    delay(2000);
}