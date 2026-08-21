// ============================================================
//  Robot.cpp
//  Zustandsmaschine – keine Navigations- oder Anzeigelogik.
//  Alle Fachlogik liegt in Navigation, Display, WiFiController.
// ============================================================

#include "Robot.h"
#include "Config.h"

using namespace Config;

// ============================================================
//  Konstruktor – Subsysteme mit Config-Werten instantiieren
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
//  Wird einmalig in setup() aufgerufen.
//  Initialisiert alle Subsysteme → Uebergang nach READY.
// ============================================================

void Robot::begin() {
    Serial.begin(115200);

    // Display
    if (!_display.begin()) {
        Serial.println(F("[Robot] FEHLER: Display nicht gefunden"));
        for (;;) {
            delay(1000);   // Yield an den Idle-Task -> verhindert Task-Watchdog-Reset
        }
    }
    _display.show("Initialisierung");

    // Navigation: Callbacks registrieren
    _navigation.setDisplayCallback([this](const String& text) {
        _display.show(text);
    });

    // Treiber
    _motor.begin();
    _sonarCenter.begin();
    _sonarLeft.begin();
    _sonarRight.begin();
    _irFloor.begin();

    delay(500);

    // WiFi: Callbacks registrieren, dann starten
    _registerWifiCallbacks();
    _wifi.begin();

    _transitionTo(State::READY, "Bereit\n" + _wifi.ipAddress());
}

// ============================================================
//  update()
//  Wird in jedem loop()-Durchlauf aufgerufen.
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
//  Zustandshandler
// ============================================================

void Robot::_handleInit() {
    // INIT wird in begin() abgehandelt.
    // Falls update() vor begin() aufgerufen wird, nichts tun.
}

void Robot::_handleReady() {
    // Warten auf Start-Befehl (kommt ueber WiFi-Callback).
    // Motoren bleiben gestoppt.
}

void Robot::_handleRun() {
    _navigation.update();
}

// ============================================================
//  Oeffentliche Zustandsuebergaenge
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
//  _transitionTo() – Zustandswechsel mit optionalem Display
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
//  Verdrahtet WiFiController-Callbacks mit Robot-Methoden.
//  Robot ist der einzige Ort, der beide kennt.
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
            Config::NavigationCfg::SPEED_LEVEL_3,
            Config::NavigationCfg::SPEED_LEVEL_4
        };
        if (stufe >= 1 && stufe <= 4) {
            _navigation.setMaxSpeed(speeds[stufe - 1]);
        }
    });

    _wifi.onParameter([this](int turn, int stopCm) {
        if (turn   > 0) _navigation.setTurnFrontThreshold(constrain(turn,   1, 400));
        if (stopCm > 0) _navigation.setStopThreshold(constrain(stopCm, 1, 400));
    });

    _wifi.onStatus([this]() -> String {
        return "State:"   + String((int)_state) +
               " Turn:"   + String(_navigation.turnFrontThreshold()) +
               " Stop:"   + String(_navigation.stopThreshold()) +
               " Speed:"  + String(_navigation.maxSpeed()) +
               " M:"      + String(_navigation.distanceCenter()) +
               " L:"      + String(_navigation.distanceLeft()) +
               " R:"      + String(_navigation.distanceRight()) +
               " BTM:"    + String(_navigation.floor()    ? 1 : 0);
    });
}