#include "TestFunctions.h"

#include <Arduino.h>
#include <Wire.h>

void testI2CScan() {
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

void testMosfetControl(MosfetControl& mosfetControl) {
    Serial.println("Testing MOSFET control...");
    Serial.print("Switching to MOSFET_OFF: ");
    Serial.println(mosfetControl.setState(MosfetState::Off) ? "Success" : "Failure");
    delay(1000);

    Serial.print("Switching to MOSFET_CHARGE: ");
    Serial.println(mosfetControl.setState(MosfetState::Charge) ? "Success" : "Failure");
    delay(1000);

    Serial.print("Switching to MOSFET_LOAD: ");
    Serial.println(mosfetControl.setState(MosfetState::Load) ? "Success" : "Failure");
    delay(1000);

    Serial.print("Switching to MOSFET_OFF: ");
    Serial.println(mosfetControl.setState(MosfetState::Off) ? "Success" : "Failure");
}

void printIna219Readings(const Ina219Readings& readings) {
    Serial.print("Bus Voltage:   ");
    Serial.print(readings.busVoltageV);
    Serial.println(" V");
    Serial.print("Shunt Voltage: ");
    Serial.print(readings.shuntVoltageMv);
    Serial.println(" mV");
    Serial.print("Load Voltage:  ");
    Serial.print(readings.loadVoltageV);
    Serial.println(" V");
    Serial.print("Current:       ");
    Serial.print(readings.currentMa);
    Serial.println(" mA");
    Serial.print("Power:         ");
    Serial.print(readings.powerMw);
    Serial.println(" mW");
    Serial.println();
}

void testIna219Readout(Ina219Monitor& ina219) {
    printIna219Readings(ina219.read());
    delay(2000);
}