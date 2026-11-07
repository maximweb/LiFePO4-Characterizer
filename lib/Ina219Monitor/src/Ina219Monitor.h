#pragma once

#include <Adafruit_INA219.h>

struct Ina219Readings {
    float busVoltageV;
    float shuntVoltageMv;
    float currentMa;
    float powerMw;
    float loadVoltageV;
};

class Ina219Monitor {
  public:
    /** Creates a monitor for the INA219 at the given I2C address. */
    explicit Ina219Monitor(uint8_t address);

    /** Starts I2C and initializes the INA219. */
    bool begin();

    /** Reads and returns the current INA219 measurements. */
    Ina219Readings read();

  private:
    Adafruit_INA219 sensor;
};