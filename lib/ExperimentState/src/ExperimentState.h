#pragma once

enum class ExperimentState {
    Idle,
    AwaitingCharge,
    Charging,
    SettlingAfterCharging,
    LoadPeriod,
    NoLoadSettlePeriod,
    SettlingAfterLowVoltage,
    EndManualStop,
    EndLowVoltage,
    EndOvercurrent,
    EndFault
};