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
                       IrSensor&         irFloor)
    : _motor(motor),
      _sonarCenter(sonarCenter),
      _sonarLeft(sonarLeft),
      _sonarRight(sonarRight),
      _irFloor(irFloor),
      _displayCb(nullptr),
      _maxSpeed(NavigationCfg::SPEED_LEVEL_1),
      _reverseDuration(NavigationCfg::REVERSE_DURATION_MS),
      _safetyDelay(NavigationCfg::SAFETY_DELAY_MS),
      _scanTurnTimeout(NavigationCfg::SCAN_TURN_TIMEOUT_MS),
      _turnFrontThreshold(NavigationCfg::FRONT_TURN_THRESHOLD_CM),
      _turnSideThreshold(NavigationCfg::SIDE_TURN_THRESHOLD_CM),
      _stopThreshold(NavigationCfg::STOP_THRESHOLD_CM),
      _margin(NavigationCfg::MARGIN_CM)
{
}

// ============================================================
//  Interne Hilfsfunktionen und Parameter
// ============================================================

void Navigation::_display(const String& text) {
    if (_displayCb) _displayCb(text);
}

// ============================================================
//  update() – ein Navigations-Zyklus
// ============================================================

void Navigation::update() {
    /*
     * Sensoren nur periodisch aktualisieren.
     *
     * Dadurch wird nicht in jedem loop()-Durchlauf eine
     * komplette Ultraschallmessung durchgeführt.
     */

    constexpr unsigned long SENSOR_INTERVAL_MS = 80;

    if (millis() - _lastSensorUpdate >= SENSOR_INTERVAL_MS) {
        _updateSensors();
        _lastSensorUpdate = millis();
    }

    if (_sensors.floor) {
        if (_state != State::EMERGENCY_STOP) {
            _transitionTo(State::EMERGENCY_STOP);
        }
        _motor.stop();
        return;
    }

    switch (_state){
        case State::DRIVING:           _updateDriving();            break;
        case State::BLOCKED_REVERSE:   _updateBlockedReverse();     break;
        case State::BLOCKED_SCAN_TURN: _updateBlockedScanAndTurn(); break;
        // SZENARIO WIRD NOCH NICHT UNTERSTÜTZT
        // case State::FLOOR_REVERSE:     _updateFloorReverse();       break;
        // case State::FLOOR_TURN:        _updateFloorTurn();          break;
        case State::EMERGENCY_STOP:    _updateEmergencyStop();      break;
    }
}

// ============================================================
// Sensoren abfragen
// ============================================================

void Navigation::_updateSensors() {
    _sensors.floor = _irFloor.isTriggered();
    _sensors.center = _sonarCenter.measureAverage(
            2,
            Ultrasonic::SAMPLE_DELAY_MS
        );

    _sensors.left = _sonarLeft.measureAverage(
            2,
            Ultrasonic::SAMPLE_DELAY_MS
        );

    _sensors.right = _sonarRight.measureAverage(
            2,
            Ultrasonic::SAMPLE_DELAY_MS
        );

    Serial.print("[Nav] L=");
    Serial.print(_sensors.left);

    Serial.print(" C=");
    Serial.print(_sensors.center);

    Serial.print(" R=");
    Serial.println(_sensors.right);
}

// ============================================================
// Driving
// ============================================================

void Navigation::_updateDriving() {

    if (_sensors.center <= _stopThreshold || 
        _sensors.left <= _stopThreshold || 
        _sensors.right <= _stopThreshold) {
        _startBlockedReverse();
        return;
    }


    /*
     * Hindernis seitlich:
     *
     * links blockiert → rechts fahren
     * rechts blockiert → links fahren
     */
/* SZENARIO WIRD NOCH NICHT UNTERSTÜTZT
    if (_leftBlocked())
    {
        _display("Kurve rechts");

        _motor.curveRight(
            _maxSpeed,
            _calcSpeed(_sensors.center)
        );

        return;
    }

    if (_rightBlocked())
    {
        _display("Kurve links");

        _motor.curveLeft(
            _maxSpeed,
            _calcSpeed(_sensors.center)
        );

        return;
    }
*/

    /*
     * Front wird enger:
     * Geschwindigkeit reduzieren.
     */

    uint8_t speed =
        _calcSpeed(_sensors.center);

    _motor.forward(speed);
}

// ============================================================
// Blockage situaton - reverse
// ============================================================

void Navigation::_startBlockedReverse() {
    _motor.stop();
    _display("Hindernis");
    _transitionTo(State::BLOCKED_REVERSE);
    _motor.backward(_maxSpeed);
}

void Navigation::_updateBlockedReverse() {
    if (!_stateElapsed(_reverseDuration)) return;
    
    _motor.stop();
    _startBlockedScanAndTurn();
}

// ============================================================
// Blockage situation - scan and turn
// ============================================================

void Navigation::_startBlockedScanAndTurn() {
 
    if (_sensors.left > _sensors.right) {
        _motor.turnRight(_maxSpeed);
    }
    else if (_sensors.right > _sensors.left) {
        _motor.turnLeft(_maxSpeed);
    }
    else {
        // Both directions are equally blocked, 
        //choose a default direction
        _motor.turnRight(_maxSpeed);
    }

    _transitionTo(State::BLOCKED_SCAN_TURN);
}

void Navigation::_updateBlockedScanAndTurn() {

    // Timeout: Falls trotz Drehen keine Richtung frei wird (z.B. in
    // einer Ecke gefangen), erneut zurücksetzen statt endlos zu drehen.
    if (_stateElapsed(_scanTurnTimeout)) {
        _motor.stop();
        _display("Kein Weg frei");
        _transitionTo(State::DRIVING);   // sauberer Ausgangszustand für den nächsten Start
        if (_stuckCb) _stuckCb();
        return;
    }

    // Spin robot as long as all sensors are free
    if (_sensors.center < _stopThreshold + _margin || 
        _sensors.left < _stopThreshold + _margin || 
        _sensors.right < _stopThreshold + _margin) {
        return;
    }

    _motor.stop();
    _display("Weiter");
    _transitionTo(State::DRIVING);
}

// ============================================================
// Emergency Stop
// ============================================================

void Navigation::_updateEmergencyStop() {
    _motor.stop();
    // Diese Funktion wird nur erreicht, wenn floorBottom bereits false ist
    // (siehe update()) – die frühere Prüfung hier war unerreichbar.

    if (_stateElapsed(_safetyDelay)){
        _display("Bereit");
        _transitionTo(State::DRIVING);
    }
}

// ============================================================
// Geschwindigkeit
// ============================================================

uint8_t Navigation::_calcSpeed(int distanceCm) const {

    if (distanceCm >= _turnFrontThreshold)
        return _maxSpeed;

    if (distanceCm <= _stopThreshold)
        return NavigationCfg::SPEED_SLOW;

    const int range =
        _turnFrontThreshold - _stopThreshold;

    const int offset =
        distanceCm - _stopThreshold;

    int speed =
        NavigationCfg::SPEED_SLOW +
        (
            (_maxSpeed -
             NavigationCfg::SPEED_SLOW)
            * offset
        ) / range;

    return static_cast<uint8_t>(
        constrain(
            speed,
            NavigationCfg::SPEED_SLOW,
            _maxSpeed
        )
    );
}

// ============================================================
//  _transitionTo() – Zustandswechsel
// ============================================================

void Navigation::_transitionTo(State next) {
    _state = next;
    _stateStartedAt = millis();

    Serial.print("[Nav] State -> ");
    Serial.println((int)next);
}

bool Navigation::_stateElapsed(unsigned long duration) const {
    return millis() - _stateStartedAt >= duration;
}

// ============================================================
// Sensorabfragen für Webendpoint /status
// ============================================================
int Navigation::distanceCenter() {
    return _sensors.center;
}

int Navigation::distanceLeft() {
    return _sensors.left;
}

int Navigation::distanceRight() {
    return _sensors.right;
}

bool Navigation::floor() {
    return _sensors.floor;
}

// ============================================================
// Public configuration
// ============================================================

void Navigation::start() {
    _transitionTo(State::DRIVING);
}


void Navigation::stop() {
    _motor.stop();
}

Navigation::State Navigation::state() const {
    return _state;
}

const Navigation::SensorSnapshot&
Navigation::sensors() const {
    return _sensors;
}