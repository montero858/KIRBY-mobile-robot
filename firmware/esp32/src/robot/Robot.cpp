// ============================================================
//  Robot.cpp
//  State machine – no navigation or display logic.
//  All domain logic lives in Navigation, Display, WiFiController.
// ============================================================

#include "Robot.h"
#include "Config.h"

using namespace Config;

// ============================================================
//  Constructor – instantiate subsystems with Config values
// ============================================================

Robot::Robot()
    : _state(State::INIT),
      _motor(
          Motor::IN1, Motor::IN2,
          Motor::IN3, Motor::IN4,
          Motor::ENA, Motor::ENB,
          Motor::PWM_CHANNEL_A, Motor::PWM_CHANNEL_B,
          Motor::PWM_FREQ, Motor::PWM_BITS),
      _sonarCenter(Ultrasonic::TRIG_CENTER, Ultrasonic::ECHO_CENTER,
                   Ultrasonic::MAX_DISTANCE_CM),
      _sonarLeft  (Ultrasonic::TRIG_LEFT,   Ultrasonic::ECHO_LEFT,
                   Ultrasonic::MAX_DISTANCE_CM),
      _sonarRight (Ultrasonic::TRIG_RIGHT,  Ultrasonic::ECHO_RIGHT,
                   Ultrasonic::MAX_DISTANCE_CM),
      _irFloor(Ir::IR_FLOOR, IrSensor::Mode::FLOOR,
                    Ir::REFLECTION_LEVEL,  Ir::DEBOUNCE_US),
      _display(DisplayCfg::SDA, DisplayCfg::SCL,
               DisplayCfg::I2C_ADDR, DisplayCfg::WIDTH, DisplayCfg::HEIGHT),
      _navigation(_motor,
                  _sonarCenter, _sonarLeft, _sonarRight,
                  _irFloor),
      _wifi(Wifi::SSID, Wifi::PASSWORD,
            Wifi::CHANNEL, Wifi::HIDDEN, Wifi::MAX_CLIENTS)
{}

// ============================================================
//  begin()
//  Called once in setup().
//  Initializes all subsystems -> transition to READY.
// ============================================================

void Robot::begin() {
    Serial.begin(115200);

    // Display
    if (!_display.begin()) {
        Serial.println(F("[Robot] FEHLER: Display nicht gefunden"));
        for (;;) {
            delay(1000);   // yield to the idle task -> prevents task watchdog reset
        }
    }
    _display.show("Initialisierung");

    // Navigation: register callbacks
    _navigation.setDisplayCallback([this](const String& text) {
        _display.show(text);
    });

    // Drivers
    _motor.begin();
    _sonarCenter.begin();
    _sonarLeft.begin();
    _sonarRight.begin();
    _irFloor.begin();

    delay(500);

    // WiFi: register callbacks, then start
    _registerWifiCallbacks();
    _wifi.begin();

    _transitionTo(State::READY, "Bereit\n" + _wifi.ipAddress());
}

// ============================================================
//  update()
//  Called on every loop() iteration.
// ============================================================

void Robot::update() {
    _wifi.handle();

    switch (_state) {
        case State::INIT:  _handleInit();  break;
        case State::READY: _handleReady(); break;
        case State::RUN:   _handleRun();   break;
    }
}

// ============================================================
//  State handlers
// ============================================================

void Robot::_handleInit() {
    // INIT is handled in begin().
    // If update() is called before begin(), do nothing.
}

void Robot::_handleReady() {
    // Waiting for the start command (arrives via WiFi callback).
    // Motors remain stopped.
}

void Robot::_handleRun() {
    _navigation.update();
}

// ============================================================
//  Public state transitions
// ============================================================

void Robot::start() {
    if (_state != State::READY) return;
    _transitionTo(State::RUN, "Fahre");
}

void Robot::stop() {
    if (_state != State::RUN) return;
    _motor.stop();
    _transitionTo(State::READY, "Bereit");
}

// ============================================================
//  _transitionTo() – state change with optional display update
// ============================================================

void Robot::_transitionTo(State next, const String& displayText) {
    _state = next;
    Serial.print(F("[Robot] State → "));
    Serial.println((int)next);
    if (displayText.length() > 0) {
        _display.show(displayText);
    }
}

// ============================================================
//  _registerWifiCallbacks()
//  Wires WiFiController callbacks to Robot methods.
//  Robot is the only place that knows both.
// ============================================================

void Robot::_registerWifiCallbacks() {

    _wifi.onStart([this]() {
        start();
    });

    _wifi.onStop([this]() {
        stop();
    });

    _wifi.onStufe([this](int stufe) {
        const uint8_t speeds[] = {
            Config::NavigationCfg::SPEED_LEVEL_1,
            Config::NavigationCfg::SPEED_LEVEL_2,
            Config::NavigationCfg::SPEED_LEVEL_3
        };
        if (stufe >= 1 && stufe <= 3) {
            _navigation.setMaxSpeed(speeds[stufe - 1]);
        }
    });

    _wifi.onParameter([this](int turn, int sideTurn, int stop,
                              int turnFactor, int reverseDuration) {
        if (turn            != WiFiController::PARAM_UNSET)
            _navigation.setTurnFrontThreshold(constrain(turn, 1, 400));
        if (sideTurn        != WiFiController::PARAM_UNSET)
            _navigation.setSideTurnThreshold(constrain(sideTurn, 1, 400));
        if (stop            != WiFiController::PARAM_UNSET)
            _navigation.setStopThreshold(constrain(stop, 1, 400));
        if (turnFactor      != WiFiController::PARAM_UNSET)
            _navigation.setTurnFactor(constrain(turnFactor, -100, 100));
        if (reverseDuration != WiFiController::PARAM_UNSET)
            _navigation.setReverseDuration(constrain(reverseDuration, 50, 5000));
    });

    _wifi.onStatus([this]() -> String {
        return "State:"   + String((int)_state) +
            " Turn:"   + String(_navigation.turnFrontThreshold()) +
            " Stop:"   + String(_navigation.stopThreshold()) +
            " Side:"   + String(_navigation.sideTurnThreshold()) +
            " TF:"     + String(_navigation.turnFactor()) +
            " Rev:"    + String(_navigation.reverseDuration()) +
            " Speed:"  + String(_navigation.maxSpeed()) +
            " M:"      + String(_navigation.distanceCenter()) +
            " L:"      + String(_navigation.distanceLeft()) +
            " R:"      + String(_navigation.distanceRight()) +
            " BTM:"    + String(_navigation.floor()    ? 1 : 0);
    });
}