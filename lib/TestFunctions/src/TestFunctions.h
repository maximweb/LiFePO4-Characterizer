#pragma once

#include <Ina219Monitor.h>
#include <MosfetControl.h>

/** Scans I2C addresses and prints any responding devices to the serial monitor. */
void testI2CScan();

/** Cycles through MOSFET states, reports results, and finishes with both off. */
void testMosfetControl(MosfetControl &mosfetControl);

/** Reads INA219 measurements and prints them to the serial monitor. */
void testIna219Readout(Ina219Monitor &ina219);