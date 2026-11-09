#include "HomeAssistant.h"

#include <ESP8266WiFi.h>

#include "config.h"

HomeAssistant *HomeAssistant::instance = nullptr;

namespace {
const char *experimentStateName(ExperimentState state) {
    switch (state) {
    case ExperimentState::Idle:
        return "idle";
    case ExperimentState::AwaitingCharge:
        return "awaiting-charge";
    case ExperimentState::Charging:
        return "charging";
    case ExperimentState::SettlingAfterCharging:
        return "settling-delay-after-charging";
    case ExperimentState::LoadPeriod:
        return "load-period";
    case ExperimentState::NoLoadSettlePeriod:
        return "no-load-settle-period";
    case ExperimentState::SettlingAfterLowVoltage:
        return "settling-after-low-voltage";
    case ExperimentState::EndManualStop:
        return "end-manual-stop";
    case ExperimentState::EndLowVoltage:
        return "end-low-voltage";
    case ExperimentState::EndOvercurrent:
        return "end-overcurrent";
    case ExperimentState::EndFault:
        return "end-fault";
    }
    return "idle";
}
} // namespace

HomeAssistant::HomeAssistant()
    : mqtt(networkClient, device), busVoltage("bus_voltage", HASensorNumber::PrecisionP2),
      shuntVoltage("shunt_voltage", HASensorNumber::PrecisionP2),
      current("current", HASensorNumber::PrecisionP3), power("power", HASensorNumber::PrecisionP2),
      loadVoltage("load_voltage", HASensorNumber::PrecisionP2),
      dischargedAh("discharged_ah", HASensorNumber::PrecisionP3),
      stateOfCharge("state_of_charge", HASensorNumber::PrecisionP1),
      experimentStateSensor("experiment_state"),
      measurementCommandStatusSensor("measurement_command_status"),
      measurementCommand("measurement_command") {
    instance = this;
}

void HomeAssistant::begin(const char *broker, uint16_t port, const char *username,
                          const char *password) {
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

    dischargedAh.setName("Discharged Charge");
    dischargedAh.setStateClass("measurement");
    dischargedAh.setUnitOfMeasurement("Ah");

    stateOfCharge.setName("Estimated State of Charge");
    stateOfCharge.setDeviceClass("battery");
    stateOfCharge.setStateClass("measurement");
    stateOfCharge.setUnitOfMeasurement("%");

    experimentStateSensor.setName("Experiment State");
    measurementCommandStatusSensor.setName("Measurement Command Status");

    measurementCommand.setName("Measurement Command");
    measurementCommand.setOptions("measuring;off");
    measurementCommand.setCurrentState(OffCommand);
    measurementCommand.onCommand(handleMeasurementCommand);

    mqtt.begin(broker, port, username, password);
}

void HomeAssistant::loop() { mqtt.loop(); }

void HomeAssistant::publishReadings(const Ina219Readings &readings) {
    busVoltage.setValue(readings.busVoltageV);
    shuntVoltage.setValue(readings.shuntVoltageMv);
    current.setValue(readings.currentMa / 1000.0f);
    power.setValue(readings.powerMw / 1000.0f);
    loadVoltage.setValue(readings.loadVoltageV);
}

void HomeAssistant::clearCoulombMetrics() {
    static_cast<HASensor &>(dischargedAh).setValue(nullptr);
    static_cast<HASensor &>(stateOfCharge).setValue(nullptr);
}

void HomeAssistant::publishDischargedAh(float dischargedAhValue) {
    dischargedAh.setValue(dischargedAhValue);
}

void HomeAssistant::publishStateOfCharge(float stateOfChargePercent) {
    stateOfCharge.setValue(stateOfChargePercent);
}

void HomeAssistant::setExperimentState(ExperimentState state) {
    experimentState = state;
    publishExperimentState();
}

void HomeAssistant::publishExperimentState() {
    experimentStateSensor.setValue(experimentStateName(experimentState));
}

void HomeAssistant::setMeasurementCommandStatus(bool measuring) {
    measurementRequested = measuring;
    publishMeasurementCommandStatus();
}

void HomeAssistant::publishMeasurementCommandStatus() {
    measurementCommandStatusSensor.setValue(measurementRequested ? "measuring" : "off");
}

int8_t HomeAssistant::takeCommand() {
    const int8_t command = pendingCommand;
    pendingCommand = NoCommand;
    return command;
}

void HomeAssistant::handleMeasurementCommand(int8_t index, HASelect *sender) {
    if (instance == nullptr || (index != MeasuringCommand && index != OffCommand)) {
        return;
    }

    instance->pendingCommand = index;
    sender->setState(index);
}