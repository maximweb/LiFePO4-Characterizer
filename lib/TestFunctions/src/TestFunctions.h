#pragma once

#include <Ina219Monitor.h>
#include <MosfetControl.h>

/** Scans I2C addresses and prints any responding devices to the serial monitor. */
void testI2CScan();

/** Cycles through MOSFET states, reports results, and finishes with both off. */
void testMosfetControl(MosfetControl &mosfetControl);

/** Prints a set of INA219 readings to the serial monitor. */
void printIna219Readings(const Ina219Readings &readings);

/** Reads INA219 measurements and prints them to the serial monitor. */
void testIna219Readout(Ina219Monitor &ina219);