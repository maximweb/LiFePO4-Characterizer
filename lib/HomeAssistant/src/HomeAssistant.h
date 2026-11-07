#pragma once

#include <ArduinoHA.h>
#include <Ina219Monitor.h>
#include <WiFiClient.h>

class HomeAssistant {
  public:
    static constexpr int8_t NoCommand = -1;
    static constexpr int8_t MeasuringCommand = 0;
    static constexpr int8_t OffCommand = 1;

    HomeAssistant();

    /** Registers the device and starts MQTT auto-discovery. */
    void begin(const char* broker, uint16_t port, const char* username, const char* password);

    /** Maintains the MQTT connection; call frequently from loop(). */
    void loop();

    /** Publishes current INA219 values and the current run labels. */
    void publishReadings(const Ina219Readings& readings);

    /** Returns the next received measurement command, or NoCommand. */
    int8_t takeCommand();

  private:
    static HomeAssistant* instance;
    static void handleMeasurementCommand(int8_t index, HASelect* sender);

    WiFiClient networkClient;
    HADevice device;
    HAMqtt mqtt;
    HASensorNumber busVoltage;
    HASensorNumber shuntVoltage;
    HASensorNumber current;
    HASensorNumber power;
    HASensorNumber loadVoltage;
    HASensor runState;
    HASelect measurementCommand;
    int8_t pendingCommand = NoCommand;
    const char* runStateValue = "off";
};