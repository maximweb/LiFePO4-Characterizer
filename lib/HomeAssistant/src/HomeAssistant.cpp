#include "HomeAssistant.h"

#include <ESP8266WiFi.h>

#include "config.h"

HomeAssistant* HomeAssistant::instance = nullptr;

HomeAssistant::HomeAssistant()
    : mqtt(networkClient, device),
      busVoltage("bus_voltage", HASensorNumber::PrecisionP2),
      shuntVoltage("shunt_voltage", HASensorNumber::PrecisionP2),
      current("current", HASensorNumber::PrecisionP3),
      power("power", HASensorNumber::PrecisionP2),
      loadVoltage("load_voltage", HASensorNumber::PrecisionP2),
      runState("run_state"),
    measurementCommand("measurement_command") {
    instance = this;
}

void HomeAssistant::begin(const char* broker, uint16_t port, const char* username,
                          const char* password) {
    uint8_t deviceId[6];
    WiFi.macAddress(deviceId);
    device.setUniqueId(deviceId, sizeof(deviceId));
    device.setName("LiFePO4 Characterizer");
    device.setManufacturer("DIY");
    device.setModel("ESP8266 + INA219");
    device.setSoftwareVersion("0.1");
    device.enableSharedAvailability();
    device.enableLastWill();

    busVoltage.setName("INA219 Bus Voltage");
    busVoltage.setDeviceClass("voltage");
    busVoltage.setStateClass("measurement");
    busVoltage.setUnitOfMeasurement("V");

    shuntVoltage.setName("INA219 Shunt Voltage");
    shuntVoltage.setDeviceClass("voltage");
    shuntVoltage.setStateClass("measurement");
    shuntVoltage.setUnitOfMeasurement("mV");

    current.setName("INA219 Current");
    current.setDeviceClass("current");
    current.setStateClass("measurement");
    current.setUnitOfMeasurement("A");

    power.setName("INA219 Power");
    power.setDeviceClass("power");
    power.setStateClass("measurement");
    power.setUnitOfMeasurement("W");

    loadVoltage.setName("INA219 Load Voltage");
    loadVoltage.setDeviceClass("voltage");
    loadVoltage.setStateClass("measurement");
    loadVoltage.setUnitOfMeasurement("V");

    runState.setName("Experiment State");

    measurementCommand.setName("Measurement Command");
    measurementCommand.setOptions("measuring;off");
    measurementCommand.setCurrentState(OffCommand);
    measurementCommand.onCommand(handleMeasurementCommand);

    mqtt.begin(broker, port, username, password);
}

void HomeAssistant::loop() { mqtt.loop(); }

void HomeAssistant::publishReadings(const Ina219Readings& readings) {
    busVoltage.setValue(readings.busVoltageV);
    shuntVoltage.setValue(readings.shuntVoltageMv);
    current.setValue(readings.currentMa / 1000.0f);
    power.setValue(readings.powerMw / 1000.0f);
    loadVoltage.setValue(readings.loadVoltageV);
    runState.setValue(runStateValue);
}

int8_t HomeAssistant::takeCommand() {
    const int8_t command = pendingCommand;
    pendingCommand = NoCommand;
    return command;
}

void HomeAssistant::handleMeasurementCommand(int8_t index, HASelect* sender) {
    if (instance == nullptr || (index != MeasuringCommand && index != OffCommand)) {
        return;
    }

    instance->pendingCommand = index;
    sender->setState(index);
    instance->runStateValue = index == MeasuringCommand ? "measuring" : "off";
}