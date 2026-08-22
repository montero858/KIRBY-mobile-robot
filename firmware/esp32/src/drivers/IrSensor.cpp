// ============================================================
//  IrSensor.cpp
// ============================================================

#include "IrSensor.h"

// ============================================================
//  Constructor
// ============================================================

IrSensor::IrSensor(uint8_t pin, Mode mode,
                   uint8_t reflectionLevel, uint16_t debounceUs)
    : _pin(pin),
      _mode(mode),
      _reflectionLevel(reflectionLevel),
      _debounceUs(debounceUs)
{}

// ============================================================
//  begin()
// ============================================================

void IrSensor::begin() {
    pinMode(_pin, INPUT);
}

// ============================================================
//  rawReflection()
//  Single reading without debouncing.
//  Returns true if a reflection is currently detected
//  (object or floor present), regardless of Mode.
// ============================================================

bool IrSensor::rawReflection() {
    return digitalRead(_pin) == _reflectionLevel;
}

// ============================================================
//  isTriggered()
//  Debounced query: both readings must agree.
//
//  Mode::OBSTACLE: alarm when reflection is present (object nearby)
//  Mode::FLOOR:    alarm when NO reflection (no floor!)
// ============================================================

bool IrSensor::isTriggered() {
    bool first  = rawReflection();
    delayMicroseconds(_debounceUs);
    bool second = rawReflection();

    // Both readings must be identical (debouncing)
    if (first != second) {
        return false;   // unstable signal – no alarm
    }

    bool reflection = first;

    switch (_mode) {
        case Mode::OBSTACLE:
            // Alarm when reflection is detected (object nearby)
            return reflection;

        case Mode::FLOOR:
            // Alarm when NO reflection (floor missing = fall risk!)
            return !reflection;

        default:
            return false;
    }
}
