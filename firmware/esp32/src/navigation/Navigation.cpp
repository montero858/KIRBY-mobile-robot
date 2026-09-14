// ============================================================
//  Navigation.cpp
// ============================================================

#include "Navigation.h"
#include "Config.h"

using namespace Config;

// ============================================================
//  Constructor
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
      _stopThreshold(NavigationCfg::STOP_THRESHOLD_SPD1_CM),
      _turnFactor(NavigationCfg::TURN_FACTOR),
      _margin(NavigationCfg::MARGIN_CM)
{
    setMaxSpeed(_maxSpeed); // initialize _lastMaxSpeed 
                            // and call _updateLimitsBasedOnSpeed()
}

// ============================================================
//  Internal helper functions and parameters
// ============================================================

void Navigation::_display(const String& text) {
    if (_displayCb) _displayCb(text);
}

// ============================================================
//  update() – one navigation cycle
// ============================================================

void Navigation::update() {
    /*
     * Only update sensors periodically.
     *
     * This avoids performing a full ultrasonic measurement on
     * every loop() iteration.
     */

    constexpr unsigned long SENSOR_INTERVAL_MS = 80;

    if (millis() - _lastSensorUpdate >= SENSOR_INTERVAL_MS) {
        _updateSensors();
        _lastSensorUpdate = millis();
    }

    if (_sensors.floor) {
        // Robot has been lifted: stop immediately, state EMERGENCY_STOP.
        // The recovery timer is deliberately NOT started yet - that
        // happens in _updateEmergencyStop() only once the floor is
        // detected again (_sensors.floor == false).
        if (_state != State::EMERGENCY_STOP) {
            _transitionTo(State::EMERGENCY_STOP);
        }
        _emergencyRecoveryArmed = false;
        _motor.stop();
        return;
    }

    switch (_state){
        case State::DRIVING:           _updateDriving();            break;
        case State::BLOCKED_REVERSE:   _updateBlockedReverse();     break;
        case State::BLOCKED_SCAN_TURN: _updateBlockedScanAndTurn(); break;
        case State::EMERGENCY_STOP:    _updateEmergencyStop();      break;
    }
}

// ============================================================
// Read sensors
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

    uint8_t speed =
        _calcSpeed(_sensors.center);

    _motor.forward(speed);
}

// ============================================================
// Blocked situation - reverse
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
// Blocked situation - scan and turn
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

    // Timeout: if no direction becomes free despite turning (e.g.
    // trapped in a corner), back up again instead of spinning forever.
    if (_stateElapsed(_scanTurnTimeout)) {
        _motor.stop();
        _display("Kein Weg frei");
        _transitionTo(State::DRIVING);   // clean starting state for the next start
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
    // This function is only reached once floorBottom is already false
    // (see update()) – the earlier check here was unreachable.

    if (!_emergencyRecoveryArmed) {
        // Floor detected again just now: start counting the delay
        // from NOW, not from the moment the robot was lifted.
        _stateStartedAt = millis();
        _emergencyRecoveryArmed = true;
        return;
    }

    if (_stateElapsed(_safetyDelay)){
        _display("Bereit");
        _transitionTo(State::DRIVING);
    }
}

// ============================================================
// Speed
// ============================================================

uint8_t Navigation::_calcSpeed(int distanceCm) const {

    // Invalid measurement (timeout / outside the measurement range):
    // do NOT treat as "path clear" (999 >= _turnFrontThreshold would
    // otherwise incorrectly result in full speed); instead treat it
    // safely like a near/uncertain distance -> slow driving.
    if (distanceCm == UltrasonicSensor::INVALID_DISTANCE)
        return NavigationCfg::SPEED_SLOW;

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

void Navigation::_updateLimitsBasedOnSpeed(uint8_t speed) {
    switch (speed) {
        case NavigationCfg::SPEED_LEVEL_1:
            _stopThreshold = NavigationCfg::STOP_THRESHOLD_SPD1_CM;
            _turnFactor = NavigationCfg::TURN_FACTOR;
            break;
        case NavigationCfg::SPEED_LEVEL_2:
            _stopThreshold = NavigationCfg::STOP_THRESHOLD_SPD2_CM;
            _turnFactor = NavigationCfg::TURN_FACTOR - 10;   
            break;
        case NavigationCfg::SPEED_LEVEL_3:
            _stopThreshold = NavigationCfg::STOP_THRESHOLD_SPD3_CM;
            _turnFactor = NavigationCfg::TURN_FACTOR - 20;
            break;
        case NavigationCfg::SPEED_LEVEL_4:
            _stopThreshold = NavigationCfg::STOP_THRESHOLD_SPD4_CM;
            _turnFactor = NavigationCfg::TURN_FACTOR - 30;
            
            break;
        default:
            // Default values for unknown speed levels
            _stopThreshold = NavigationCfg::STOP_THRESHOLD_SPD4_CM;
            _turnFactor = NavigationCfg::TURN_FACTOR;

        _innerSpeed = (speed * _turnFactor) / 100;
    }
}

// ============================================================
//  _transitionTo() – state change
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
// Sensor queries for the /status web endpoint
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
