// ============================================================
//  Navigation.cpp
// ============================================================

#include "Navigation.h"
#include "Config.h"

using namespace Config;

// ============================================================
//  Konstruktor
// ============================================================

Navigation::Navigation(MotorDriver&      motor,
                       UltrasonicSensor& sonarCenter,
                       UltrasonicSensor& sonarLeft,
                       UltrasonicSensor& sonarRight,
                       IrSensor&         irFloorLeft,
                       IrSensor&         irFloorRight)
    : _motor(motor),
      _sonarCenter(sonarCenter),
      _sonarLeft(sonarLeft),
      _sonarRight(sonarRight),
      _irFloorLeft(irFloorLeft),
      _irFloorRight(irFloorRight),
      _displayCb(nullptr),
      _maxSpeed(NavigationCfg::SPEED_LEVEL_1),
      _slowThreshold(NavigationCfg::SLOW_THRESHOLD_CM),
      _stopThreshold(NavigationCfg::STOP_THRESHOLD_CM),
      _turnSpeed(NavigationCfg::TURN_SPEED)
{}

// ============================================================
//  Interne Hilfsfunktionen
// ============================================================

void Navigation::_display(const String& text) {
    if (_displayCb) _displayCb(text);
}

uint8_t Navigation::_calcSpeed(int distanceCm) {
    if (distanceCm >= _slowThreshold) return _maxSpeed;
    if (distanceCm <= _stopThreshold) return NavigationCfg::SPEED_MIN;

    int range  = _slowThreshold - _stopThreshold;
    int offset = distanceCm     - _stopThreshold;
    int speed  = NavigationCfg::SPEED_MIN +
                 (_maxSpeed - NavigationCfg::SPEED_MIN) * offset / range;
    return (uint8_t)constrain(speed, NavigationCfg::SPEED_MIN, (int)_maxSpeed);
}

// ============================================================
//  Sensorabfragen
// ============================================================

int  Navigation::distanceCenter()  { return _sonarCenter.measureAverage(2, Ultrasonic::SAMPLE_DELAY_MS); }
int  Navigation::distanceLeft()    { return _sonarLeft.measureAverage(2,   Ultrasonic::SAMPLE_DELAY_MS); }
int  Navigation::distanceRight()   { return _sonarRight.measureAverage(2,  Ultrasonic::SAMPLE_DELAY_MS); }
bool Navigation::floorFrontLeft()  { return _irFloorLeft.isTriggered();  }
bool Navigation::floorFrontRight() { return _irFloorRight.isTriggered(); }

// ============================================================
//  update() – ein Navigations-Zyklus
// ============================================================

void Navigation::update() {
    bool fallL = floorFrontLeft();
    bool fallR = floorFrontRight();
    int  dC    = distanceCenter();
    int  dL    = distanceLeft();
    int  dR    = distanceRight();

    // Prioritaet 1: Absturz vorne
    if (fallL || fallR) {
        _routineFloorFront();
        return;
    }

    // Prioritaet 2: Hindernis (ein Sensor unter STOP-Schwelle)
    if (dC < _stopThreshold ||
        dL < _stopThreshold ||
        dR < _stopThreshold) {
        _display("BLOCKIERT!");
        _motor.stop();
        delay(200);
        scanAndTurn();
        return;
    }

    // Prioritaet 3: Adaptive Geschwindigkeit vorwaerts
    int     minDist = min({dC, dL, dR});
    uint8_t spd     = _calcSpeed(minDist);
    _display("Fahre v:" + String(spd) +
             "\nL:" + String(dL) +
             " M:" + String(dC) +
             " R:" + String(dR));
    _motor.forward(spd);
}

// ============================================================
//  scanAndTurn()
//  Sensoren auslesen, freie Richtung bestimmen, drehen.
// ============================================================

void Navigation::scanAndTurn() {
    _motor.stop();
    delay(200);

    int dL = distanceLeft();
    int dC = distanceCenter();
    int dR = distanceRight();

    Serial.print("[Nav] Scan L:"); Serial.print(dL);
    Serial.print(" M:");           Serial.print(dC);
    Serial.print(" R:");           Serial.println(dR);

    // Alle Seiten blockiert: umkehren und 180 Grad drehen
    if (dL < _stopThreshold &&
        dC < _stopThreshold &&
        dR < _stopThreshold) {
        _display("Umkehren!");
        _motor.backward(NavigationCfg::SPEED_SLOW);
        delay(600);
        _motor.stop();
        delay(200);
        _motor.turnRight(_turnSpeed);
        delay(NavigationCfg::TURN_90_DURATION_MS * 2);
        _motor.stop();
        return;
    }

    // Freiste Seite waehlen
    if (dL >= dC && dL >= dR) {
        _display("Drehe links");
        _motor.turnLeft(_turnSpeed);
    } else if (dR >= dC && dR >= dL) {
        _display("Drehe rechts");
        _motor.turnRight(_turnSpeed);
    } else {
        _display("Weiter");  // Mitte am freisten
    }
    delay(NavigationCfg::TURN_DURATION_MS);
    _motor.stop();
    delay(200);
}

// ============================================================
//  _routineFloorFront() – Absturzsicherung vorne
// ============================================================

void Navigation::_routineFloorFront() {
    bool fallL = floorFrontLeft();
    bool fallR = floorFrontRight();

    _display("Abgrund vorne!\nWeiche aus...");
    _motor.stop();
    delay(200);
    _motor.backward(NavigationCfg::SPEED_SLOW);
    delay(NavigationCfg::REVERSE_DURATION_MS);
    _motor.stop();
    delay(200);

    if (fallL && fallR) {
        // Beide ausgeloest: vollstaendigen Scan durchfuehren
        scanAndTurn();
    } else if (fallR) {
        _display("Drehe links");
        _motor.turnLeft(_turnSpeed);
        delay(NavigationCfg::TURN_90_DURATION_MS);
    } else {
        _display("Drehe rechts");
        _motor.turnRight(_turnSpeed);
        delay(NavigationCfg::TURN_90_DURATION_MS);
    }
    _motor.stop();
    delay(200);
}
