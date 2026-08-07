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
      _irFloorLeft (Ir::FLOOR_FRONT_LEFT,  IrSensor::Mode::FLOOR,
                    Ir::REFLECTION_LEVEL,  Ir::DEBOUNCE_US),
      _irFloorRight(Ir::FLOOR_FRONT_RIGHT, IrSensor::Mode::FLOOR,
                    Ir::REFLECTION_LEVEL,  Ir::DEBOUNCE_US),
      _irFloorBottom(Ir::FLOOR_BOTTOM,     IrSensor::Mode::FLOOR,
                    Ir::REFLECTION_LEVEL,  Ir::DEBOUNCE_US),
      _display(DisplayCfg::SDA, DisplayCfg::SCL,
               DisplayCfg::I2C_ADDR, DisplayCfg::WIDTH, DisplayCfg::HEIGHT),
      _navigation(_motor,
                  _sonarCenter, _sonarLeft, _sonarRight,
                  _irFloorLeft, _irFloorRight, _irFloorBottom),
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
        for (;;);  // Ohne Display nicht weiterfahren
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
    _irFloorLeft.begin();
    _irFloorRight.begin();
    _irFloorBottom.begin();

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
        case State::STOP:  _handleStop();  break;
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

void Robot::_handleStop() {
    // Notfall-Stopp aktiv – warten auf reset().
    // Motoren bereits in emergencyStop() gestoppt.
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

void Robot::reset() {
    if (_state != State::STOP) return;
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

    _wifi.onScan([this]() {
        if (_state == State::RUN || _state == State::READY) {
            _motor.stop();
            delay(200);
            //_navigation.scanAndTurn();
            // TODO: Button und Funktion evtl. entfernen
        }
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

    _wifi.onParameter([this](int slow, int stopCm, int turn) {
        if (slow   > 0) _navigation.setTurnFrontThreshold(constrain(slow,   1, 400));
        if (stopCm > 0) _navigation.setStopThreshold(constrain(stopCm, 1, 400));
        if (turn   > 0) _navigation.setTurnSpeed((uint8_t)constrain(turn, 0, 255));
    });

    _wifi.onStatus([this]() -> String {
        return "State:"   + String((int)_state) +
               " Turn:"   + String(_navigation.turnFrontThreshold()) +
               " Stop:"   + String(_navigation.stopThreshold()) +
               " Speed:"  + String(_navigation.maxSpeed()) +
               " M:"      + String(_navigation.distanceCenter()) +
               " L:"      + String(_navigation.distanceLeft()) +
               " R:"      + String(_navigation.distanceRight()) +
               " FL:"     + String(_navigation.floorFrontLeft()  ? 1 : 0) +
               " FR:"     + String(_navigation.floorFrontRight() ? 1 : 0) +
               " BTM:"    + String(_navigation.floorBottom()    ? 1 : 0);
    });
}