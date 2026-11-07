#include "MosfetControl.h"

MosfetControl::MosfetControl(uint8_t chargePin, uint8_t loadPin)
    : chargePin(chargePin), loadPin(loadPin) {}

void MosfetControl::begin() {
    digitalWrite(chargePin, LOW);
    digitalWrite(loadPin, LOW);
    pinMode(chargePin, OUTPUT);
    pinMode(loadPin, OUTPUT);
    digitalWrite(chargePin, LOW);
    digitalWrite(loadPin, LOW);
}

bool MosfetControl::setState(MosfetState state) {
    bool stateConfirmed = false;

    switch (state) {
    case MosfetState::Off:
        digitalWrite(chargePin, LOW);
        digitalWrite(loadPin, LOW);
        delay(10);
        stateConfirmed = digitalRead(chargePin) == LOW && digitalRead(loadPin) == LOW;
        break;

    case MosfetState::Charge:
        digitalWrite(loadPin, LOW);
        delay(10);
        if (digitalRead(loadPin) == LOW) {
            digitalWrite(chargePin, HIGH);
            delay(10);
            stateConfirmed = digitalRead(chargePin) == HIGH && digitalRead(loadPin) == LOW;
        }
        break;

    case MosfetState::Load:
        digitalWrite(chargePin, LOW);
        delay(10);
        if (digitalRead(chargePin) == LOW) {
            digitalWrite(loadPin, HIGH);
            delay(10);
            stateConfirmed = digitalRead(chargePin) == LOW && digitalRead(loadPin) == HIGH;
        }
        break;
    }

    if (stateConfirmed) {
        return true;
    }

    digitalWrite(chargePin, LOW);
    digitalWrite(loadPin, LOW);
    delay(10);
    return false;
}