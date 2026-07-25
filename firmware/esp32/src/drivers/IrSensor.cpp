// ============================================================
//  IrSensor.cpp
// ============================================================

#include "IrSensor.h"

// ============================================================
//  Konstruktor
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
//  Einzellesung ohne Entstörung.
//  Gibt true zurück wenn gerade eine Reflexion erkannt wird
//  (Objekt oder Boden vorhanden), unabhängig vom Mode.
// ============================================================

bool IrSensor::rawReflection() {
    return digitalRead(_pin) == _reflectionLevel;
}

// ============================================================
//  isTriggered()
//  Entstörte Abfrage: beide Lesungen müssen übereinstimmen.
//
//  Mode::OBSTACLE: Alarm wenn Reflexion vorhanden (Objekt nah)
//  Mode::FLOOR:    Alarm wenn KEINE Reflexion (kein Boden!)
// ============================================================

bool IrSensor::isTriggered() {
    bool first  = rawReflection();
    delayMicroseconds(_debounceUs);
    bool second = rawReflection();

    // Beide Lesungen müssen identisch sein (Entstörung)
    if (first != second) {
        return false;   // Instabiles Signal – kein Alarm
    }

    bool reflection = first;

    switch (_mode) {
        case Mode::OBSTACLE:
            // Alarm wenn Reflexion erkannt wird (Objekt nah)
            return reflection;

        case Mode::FLOOR:
            // Alarm wenn KEINE Reflexion (Boden fehlt = Absturz!)
            return !reflection;

        default:
            return false;
    }
}
