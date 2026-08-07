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
                       IrSensor&         irFloorRight,
                       IrSensor&         irFloorBottom)
    : _motor(motor),
      _sonarCenter(sonarCenter),
      _sonarLeft(sonarLeft),
      _sonarRight(sonarRight),
      _irFloorLeft(irFloorLeft),
      _irFloorRight(irFloorRight),
      _irFloorBottom(irFloorBottom),
      _displayCb(nullptr),
      _maxSpeed(NavigationCfg::SPEED_LEVEL_1),
      _turnFrontThreshold(NavigationCfg::FRONT_TURN_THRESHOLD_CM),
      _turnSideThreshold(NavigationCfg::SIDE_TURN_THRESHOLD_CM),
      _stopThreshold(NavigationCfg::STOP_THRESHOLD_CM),
      _turnSpeed(NavigationCfg::TURN_SPEED),
      _emergencyStopTriggered(false)
{}

// ============================================================
//  Interne Hilfsfunktionen und Parameter
// ============================================================

void Navigation::_display(const String& text) {
    if (_displayCb) _displayCb(text);
}

uint8_t Navigation::_calcSpeed(int distanceCm) {
    if (distanceCm >= _turnFrontThreshold) return _maxSpeed;
    if (distanceCm <= _stopThreshold) return NavigationCfg::SPEED_MIN;

    int range  = _turnFrontThreshold - _stopThreshold;
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
bool Navigation::floorFrontLeft()  { return _irFloorLeft.isTriggered();   }
bool Navigation::floorFrontRight() { return _irFloorRight.isTriggered();  }
bool Navigation::floorBottom()     { return _irFloorBottom.isTriggered(); }

// ============================================================
//  update() – ein Navigations-Zyklus
//
//  Prioritaeten (hoch → niedrig):
//    0. FLOOR_BOTTOM    → Sofortstopp
//    1. Absturz vorne   → Ausweichroutine
//    2. Hindernis nah   → Scan + Drehen
//    3. Freie Fahrt     → Adaptive Geschwindigkeit
// ============================================================

void Navigation::update() {

    // ── Prioritaet 0: Bodensensor unten ──────────────────────
    // Hoechste Prioritaet – kein delay, kein Scan, sofort Stopp.
    // Typischer Ausloser: Roboter wird angehoben oder faellt.
    if (floorBottom()) {
        _emergencyStopTriggered = true;
        _motor.stop();
        _display("NOTSTOPP\nBoden!");
        Serial.println(F("[Nav] FLOOR_BOTTOM ausgeloest → Emergency Stop"));
        return;
    }
    else {
        if (_emergencyStopTriggered) {
            delay(2000); // Wartezeit, bevor der Notstopp aufgehoben wird
            _emergencyStopTriggered = false;
            return;
        }
    }

    // ── Prioritaet 1: Absturz vorne ──────────────────────────
    bool fallL = floorFrontLeft();
    bool fallR = floorFrontRight();
    if (fallL || fallR) {
        _routineFloorFront(fallL, fallR);
        return;
    }

    // ── Prioritaet 2: Hindernis (Sensor unter Stop-Schwelle) ─
    int dC = distanceCenter();
    int dL = distanceLeft();
    int dR = distanceRight();

    if (dC < _stopThreshold ||
        dL < _stopThreshold ||
        dR < _stopThreshold) {
        _display("BLOCKIERT!");
        _motor.stop();
        delay(200);
        scanAndTurn();
        return;
    }
/*
    // ── Prioritaet 3: Adaptive Kurvenfahrt ───────────────────
    if (dL < _turnSideThreshold) {
        _display("Kurve rechts");
        _motor.curveRight(_maxSpeed, _calcSpeed(dC));
        return;
    } 
    else if (dR < _turnSideThreshold) {
        _display("Kurve links");
        _motor.curveLeft(_maxSpeed, _calcSpeed(dC));
        return; 
    }
    else if (dC < _turnFrontThreshold) {
        if (dL < dR) {
            _display("Kurve rechts");
            _motor.curveRight(_maxSpeed, _calcSpeed(dC));
        } else {
            _display("Kurve links");
            _motor.curveLeft(_maxSpeed, _calcSpeed(dC));
        }
        return;
    }
*/
    // ── Prioritaet 4: Normales Fahren ────────────────────────
    _motor.forward(_maxSpeed);
}


// ============================================================
//  scanAndTurn()
// ============================================================

void Navigation::scanAndTurn() {

    _motor.stop();
    delay(200);

    _display("Umkehren!");
    _motor.backward(NavigationCfg::SPEED_SLOW);
    delay(600);
    _motor.stop();
    delay(200);

    int dC = distanceCenter();
    int dL = distanceLeft();
    int dR = distanceRight();
    int timeout = 20000; // 20 Sekunden Timeout
    int cnt = 0;
    int testOffset = 30;

    Serial.print("[Nav] Scan L:"); Serial.print(dL);
    Serial.print(" M:");           Serial.print(dC);
    Serial.print(" R:");           Serial.println(dR);

    if (dC <= dL && dC <= dR){
        // Drehrichtung egal, daher links
        _display("Drehe links");
        _motor.turnLeft(_turnSpeed);
    }
    else if (dL <= dC && dL <= dR) {
        _display("Drehe links");
        _motor.turnLeft(_turnSpeed);
    } 
    else {
        _display("Drehe rechts");
        _motor.turnRight(_turnSpeed);
    } 

    while (cnt < timeout) {
        dC = distanceCenter();
        dL = distanceLeft();
        dR = distanceRight();

        Serial.print("[Nav] Scan L:"); Serial.print(dL);
        Serial.print(" M:");           Serial.print(dC);
        Serial.print(" R:");           Serial.println(dR);

        if (dC >= _stopThreshold  + testOffset &&
            dL >= _stopThreshold + testOffset &&
            dR >= _stopThreshold + testOffset) {
            break;
        }
        cnt += 1; 
    }
    _motor.stop();
    delay(200);
}

// ============================================================
//  _routineFloorFront() – Absturzsicherung vorne
// ============================================================

void Navigation::_routineFloorFront(bool fallL, bool fallR) {
    
    _display("Abgrund vorne!\nWeiche aus...");
    _motor.stop();
    delay(200);
    _motor.backward(NavigationCfg::SPEED_SLOW);
    delay(NavigationCfg::REVERSE_DURATION_MS);
    _motor.stop();
    delay(200);

    if (fallL && fallR) {
        _display("Drehe links");
        _motor.turnLeft(_turnSpeed);
        delay(NavigationCfg::TURN_90_DURATION_MS);
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