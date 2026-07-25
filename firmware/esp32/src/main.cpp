// ============================================================
//  main.cpp – Einstiegspunkt
//  Keine Logik, keine Pins, keine Konstanten.
// ============================================================

#include <Arduino.h>
#include "Robot.h"

Robot kirby;

void setup() {
    kirby.begin();
}

void loop() {
    kirby.update();
}