#include "ExperimentController.h"

#include <math.h>

#include "config.h"

namespace {
uint32_t timedStateDuration(ExperimentState state) {
    switch (state) {
    case ExperimentState::SettlingAfterCharging:
        return POST_CHARGE_SETTLE_MS;
    case ExperimentState::LoadPeriod:
        return LOAD_PERIOD_MS;
    case ExperimentState::NoLoadSettlePeriod:
        return NO_LOAD_SETTLE_PERIOD_MS;
    case ExperimentState::SettlingAfterLowVoltage:
        return POST_CUTOFF_SETTLE_MS;
    default:
        return 0;
    }
}

const char *stateName(ExperimentState state) {
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
    return "unknown";
}

void printMinutesAndSeconds(uint32_t milliseconds) {
    const uint32_t totalSeconds = milliseconds / 1000UL;
    Serial.printf("%02lu:%02lu", totalSeconds / 60UL, totalSeconds % 60UL);
}
} // namespace

ExperimentController::ExperimentController(MosfetControl &mosfetControl,
                                           HomeAssistant &homeAssistant)
    : mosfetControl(mosfetControl), homeAssistant(homeAssistant) {}

void ExperimentController::handleCommand(uint32_t now, int8_t command) {
    if (command == HomeAssistant::OffCommand) {
        if (currentState == ExperimentState::SettlingAfterLowVoltage) {
            Serial.println("Low-voltage stop already active; completing recovery rest");
            return;
        }
        if (currentState == ExperimentState::Idle || isTerminal()) {
            mosfetControl.setState(MosfetState::Off);
            return;
        }

        Serial.println("Experiment stopped manually");
        finish(ExperimentState::EndManualStop, now);
        return;
    }

    if (command != HomeAssistant::MeasuringCommand) {
        return;
    }

    if (currentState != ExperimentState::Idle) {
        Serial.println("Start ignored: reset the controller before another run");
        return;
    }

    chargeStartCount = 0;
    chargeTerminationCount = 0;
    lowVoltageCount = 0;
    if (!mosfetControl.setState(MosfetState::Charge)) {
        Serial.println("Failed to enable charger path");
        fail(now);
        return;
    }

    Serial.println("Waiting for lab supply charging current");
    transitionTo(ExperimentState::AwaitingCharge, now);
}

void ExperimentController::update(uint32_t now) {
    switch (currentState) {
    case ExperimentState::SettlingAfterCharging:
        if (now - stateStartedAt >= POST_CHARGE_SETTLE_MS) {
            if (!mosfetControl.setState(MosfetState::Load)) {
                fail(now);
                return;
            }
            Serial.println("Load period started");
            transitionTo(ExperimentState::LoadPeriod, now);
        }
        break;

    case ExperimentState::LoadPeriod:
        if (now - stateStartedAt >= LOAD_PERIOD_MS) {
            if (!mosfetControl.setState(MosfetState::Off)) {
                fail(now);
                return;
            }
            Serial.println("No-load settle period started");
            transitionTo(ExperimentState::NoLoadSettlePeriod, now);
        }
        break;

    case ExperimentState::NoLoadSettlePeriod:
        if (now - stateStartedAt >= NO_LOAD_SETTLE_PERIOD_MS) {
            if (!mosfetControl.setState(MosfetState::Load)) {
                fail(now);
                return;
            }
            Serial.println("Load period started");
            transitionTo(ExperimentState::LoadPeriod, now);
        }
        break;

    case ExperimentState::SettlingAfterLowVoltage:
        if (now - stateStartedAt >= POST_CUTOFF_SETTLE_MS) {
            Serial.println("Low-voltage recovery rest complete");
            transitionTo(ExperimentState::EndLowVoltage, now);
        }
        break;

    default:
        break;
    }
}

void ExperimentController::processReadings(uint32_t now, const Ina219Readings &readings) {
    if (currentState == ExperimentState::AwaitingCharge) {
        if (readings.currentMa <= -CHARGE_START_CURRENT_MA) {
            if (chargeStartCount < CHARGE_START_READINGS) {
                chargeStartCount++;
            }
        } else {
            chargeStartCount = 0;
        }

        if (chargeStartCount >= CHARGE_START_READINGS) {
            Serial.println("Charging current confirmed");
            chargeTerminationCount = 0;
            transitionTo(ExperimentState::Charging, now);
        }
        return;
    }

    if (currentState == ExperimentState::Charging) {
        const bool chargeVoltageAboveNominal = readings.loadVoltageV > CHARGE_MIN_VOLTAGE_V;
        const bool chargeCurrentLow =
            readings.currentMa < 0.0f && readings.currentMa >= -CHARGE_TERMINATION_CURRENT_MA;

        if (chargeVoltageAboveNominal && chargeCurrentLow) {
            if (chargeTerminationCount < CHARGE_TERMINATION_READINGS) {
                chargeTerminationCount++;
            }
        } else {
            chargeTerminationCount = 0;
        }

        if (chargeTerminationCount >= CHARGE_TERMINATION_READINGS) {
            if (!mosfetControl.setState(MosfetState::Off)) {
                fail(now);
                return;
            }
            Serial.println("Charge complete; switch the lab supply off");
            transitionTo(ExperimentState::SettlingAfterCharging, now);
        }
        return;
    }

    if (currentState == ExperimentState::LoadPeriod) {
        const float currentMa = fabsf(readings.currentMa);
        const float shuntVoltageMv = fabsf(readings.shuntVoltageMv);

        if (currentMa >= DISCHARGE_SINGLE_SAMPLE_LIMIT_MA ||
            shuntVoltageMv >= INA219_SHUNT_SATURATION_GUARD_MV) {
            Serial.println("Immediate discharge overcurrent cutoff");
            finish(ExperimentState::EndOvercurrent, now);
            return;
        }

        if (currentMa >= DISCHARGE_SUSTAINED_LIMIT_MA) {
            if (overcurrentCount < DISCHARGE_SUSTAINED_LIMIT_READINGS) {
                overcurrentCount++;
            }
        } else {
            overcurrentCount = 0;
        }

        if (overcurrentCount >= DISCHARGE_SUSTAINED_LIMIT_READINGS) {
            Serial.println("Sustained discharge overcurrent cutoff");
            finish(ExperimentState::EndOvercurrent, now);
            return;
        }
    }

    if (currentState != ExperimentState::LoadPeriod &&
        currentState != ExperimentState::NoLoadSettlePeriod) {
        return;
    }

    if (readings.loadVoltageV <= HARD_LOW_VOLTAGE_CUTOFF_V) {
        Serial.println("Hard low-voltage cutoff reached");
        beginLowVoltageSettle(now);
        return;
    }

    if (readings.loadVoltageV <= LOW_VOLTAGE_CUTOFF_V) {
        if (lowVoltageCount < LOW_VOLTAGE_CUTOFF_READINGS) {
            lowVoltageCount++;
        }
    } else {
        lowVoltageCount = 0;
    }

    if (lowVoltageCount >= LOW_VOLTAGE_CUTOFF_READINGS) {
        Serial.println("Low-voltage cutoff confirmed");
        beginLowVoltageSettle(now);
    }
}

void ExperimentController::printStatus(uint32_t now) const {
    Serial.print("Experiment state: ");
    Serial.print(stateName(currentState));

    const uint32_t duration = timedStateDuration(currentState);
    if (duration > 0) {
        const uint32_t elapsed = now - stateStartedAt;
        const uint32_t remaining = elapsed < duration ? duration - elapsed : 0;
        Serial.print(" | elapsed ");
        printMinutesAndSeconds(elapsed);
        Serial.print(" | remaining ");
        printMinutesAndSeconds(remaining);
    } else if (currentState == ExperimentState::AwaitingCharge) {
        Serial.printf(" | elapsed %lu s | waiting for current <= -%.0f mA",
                      (now - stateStartedAt) / 1000UL, CHARGE_START_CURRENT_MA);
    } else if (currentState == ExperimentState::Charging) {
        Serial.printf(
            " | elapsed %lu s | end at I in [-%.1f, 0) mA and V > %.2f V (%u samples)",
            (now - stateStartedAt) / 1000UL, CHARGE_TERMINATION_CURRENT_MA,
            CHARGE_MIN_VOLTAGE_V, static_cast<unsigned int>(CHARGE_TERMINATION_READINGS));
    }

    Serial.println();
}

void ExperimentController::transitionTo(ExperimentState state, uint32_t now) {
    currentState = state;
    stateStartedAt = now;
    if (state == ExperimentState::LoadPeriod) {
        overcurrentCount = 0;
    }
    homeAssistant.setExperimentState(state);
}

void ExperimentController::finish(ExperimentState endState, uint32_t now) {
    if (!mosfetControl.setState(MosfetState::Off)) {
        endState = ExperimentState::EndFault;
        Serial.println("Failed to confirm MOSFETs off");
    }
    transitionTo(endState, now);
}

void ExperimentController::beginLowVoltageSettle(uint32_t now) {
    if (!mosfetControl.setState(MosfetState::Off)) {
        fail(now);
        return;
    }
    transitionTo(ExperimentState::SettlingAfterLowVoltage, now);
}

void ExperimentController::fail(uint32_t now) {
    mosfetControl.setState(MosfetState::Off);
    transitionTo(ExperimentState::EndFault, now);
}

bool ExperimentController::isTerminal() const {
    return currentState == ExperimentState::EndManualStop ||
           currentState == ExperimentState::EndLowVoltage ||
           currentState == ExperimentState::EndOvercurrent ||
           currentState == ExperimentState::EndFault;
}