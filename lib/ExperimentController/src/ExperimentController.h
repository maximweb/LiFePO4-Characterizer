#pragma once

#include <Arduino.h>
#include <ExperimentState.h>
#include <HomeAssistant.h>
#include <Ina219Monitor.h>
#include <MosfetControl.h>

class ExperimentController {
  public:
    ExperimentController(MosfetControl &mosfetControl, HomeAssistant &homeAssistant);

    /** Handles one Home Assistant command. */
    void handleCommand(uint32_t now, int8_t command);

    /** Advances timed state transitions. */
    void update(uint32_t now);

    /** Evaluates a new sensor sample for charge completion and voltage cutoff. */
    void processReadings(uint32_t now, const Ina219Readings &readings);

    /** Prints the current state and timing information to the serial monitor. */
    void printStatus(uint32_t now) const;

  private:
    void transitionTo(ExperimentState state, uint32_t now);
    void finish(ExperimentState endState, uint32_t now);
    void beginLowVoltageSettle(uint32_t now);
    void fail(uint32_t now);
    bool isTerminal() const;

    MosfetControl &mosfetControl;
    HomeAssistant &homeAssistant;
    ExperimentState currentState = ExperimentState::Idle;
    uint32_t stateStartedAt = 0;
    uint8_t chargeStartCount = 0;
    uint8_t chargeTerminationCount = 0;
    uint8_t lowVoltageCount = 0;
    uint8_t overcurrentCount = 0;
};