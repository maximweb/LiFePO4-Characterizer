#pragma once

#include <ArduinoHA.h>
#include <ExperimentState.h>
#include <Ina219Monitor.h>
#include <WiFiClient.h>

class HomeAssistant {
  public:
    static constexpr int8_t NoCommand = -1;
    static constexpr int8_t MeasuringCommand = 0;
    static constexpr int8_t OffCommand = 1;

    HomeAssistant();

    /** Registers the device and starts MQTT auto-discovery. */
    void begin(const char *broker, uint16_t port, const char *username, const char *password);

    /** Maintains the MQTT connection; call frequently from loop(). */
    void loop();

    /** Publishes current INA219 values. */
    void publishReadings(const Ina219Readings &readings);

    /** Clears retained coulomb-counter values before a new experiment. */
    void clearCoulombMetrics();

    /** Publishes the net coulomb count in Ah. */
    void publishDischargedAh(float dischargedAh);

    /** Publishes the estimated state of charge in percent. */
    void publishStateOfCharge(float stateOfChargePercent);

    /** Updates the experiment state independently from HA commands. */
    void setExperimentState(ExperimentState state);

    /** Publishes the current experiment state independently from readings. */
    void publishExperimentState();

    /** Updates and publishes whether the measurement command is off or measuring. */
    void setMeasurementCommandStatus(bool measuring);

    /** Republishes the last measurement command status. */
    void publishMeasurementCommandStatus();

    /** Returns the next received measurement command, or NoCommand. */
    int8_t takeCommand();

  private:
    static HomeAssistant *instance;
    static void handleMeasurementCommand(int8_t index, HASelect *sender);

    WiFiClient networkClient;
    HADevice device;
    HAMqtt mqtt;
    HASensorNumber busVoltage;
    HASensorNumber shuntVoltage;
    HASensorNumber current;
    HASensorNumber power;
    HASensorNumber loadVoltage;
    HASensorNumber dischargedAh;
    HASensorNumber stateOfCharge;
    HASensor experimentStateSensor;
    HASensor measurementCommandStatusSensor;
    HASelect measurementCommand;
    int8_t pendingCommand = NoCommand;
    ExperimentState experimentState = ExperimentState::Idle;
    bool measurementRequested = false;
};