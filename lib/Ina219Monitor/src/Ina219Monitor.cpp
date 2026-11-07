#include "Ina219Monitor.h"

#include <Wire.h>

Ina219Monitor::Ina219Monitor(uint8_t address) : sensor(address) {}

bool Ina219Monitor::begin() {
    Wire.begin();
    return sensor.begin();
}

Ina219Readings Ina219Monitor::read() {
    Ina219Readings readings;
    readings.busVoltageV = sensor.getBusVoltage_V();
    readings.shuntVoltageMv = sensor.getShuntVoltage_mV();
    readings.currentMa = sensor.getCurrent_mA();
    readings.powerMw = sensor.getPower_mW();
    readings.loadVoltageV = readings.busVoltageV + (readings.shuntVoltageMv / 1000);
    return readings;
}