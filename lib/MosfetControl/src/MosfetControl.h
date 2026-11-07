#pragma once

#include <Arduino.h>

/** Selects which MOSFET path is enabled. */
enum class MosfetState { Off, Charge, Load };

/** Controls the mutually exclusive charger and load MOSFETs. */
class MosfetControl {
  public:
    /** Stores the GPIO pins for the charger and load MOSFETs. */
    MosfetControl(uint8_t chargePin, uint8_t loadPin);

    /** Configures both pins and starts with both MOSFETs off. */
    void begin();

    /** Sets a MOSFET state; on failure, turns both MOSFETs off. */
    bool setState(MosfetState state);

  private:
    uint8_t chargePin;
    uint8_t loadPin;
};