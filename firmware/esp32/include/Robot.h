// ============================================================
//  Robot.h
//  Top-level layer: state machine and wiring of all
//  subsystems.
//
//  States:
//    INIT   – hardware is being initialized (begin())
//    READY  – ready, waiting for start command (motors off)
//    RUN    – autonomous driving active (Navigation::update())
//
//  Allowed transitions:
//    INIT  → READY  (automatically after begin())
//    READY → RUN    (start() / web /start)
//    RUN   → READY  (stop()  / web /stopp)
// ============================================================

#pragma once

#include <Arduino.h>
#include "MotorDriver.h"
#include "UltrasonicSensor.h"
#include "IrSensor.h"
#include "Display.h"
#include "Navigation.h"
#include "WiFiController.h"

class Robot {
public:
    enum class State { INIT, READY, RUN };

    Robot();

    // Call from Arduino setup().
    // Initializes all subsystems, then transitions to READY.
    void begin();

    // Call from Arduino loop().
    void update();

    // --------------------------------------------------------
    //  State transitions
    // --------------------------------------------------------
    void start();         // READY → RUN
    void stop();          // RUN   → READY

    State state() const { return _state; }

    // Navigation exposed publicly for web server callbacks
    Navigation& navigation() { return _navigation; }

private:
    State _state;

    // --------------------------------------------------------
    //  Subsystems
    // --------------------------------------------------------
    MotorDriver      _motor;
    UltrasonicSensor _sonarCenter;
    UltrasonicSensor _sonarLeft;
    UltrasonicSensor _sonarRight;
    IrSensor         _irFloor;   // highest priority – emergency stop
    Display          _display;
    Navigation       _navigation;
    WiFiController   _wifi;

    // --------------------------------------------------------
    //  State handlers (one handler per state)
    // --------------------------------------------------------
    void _handleInit();
    void _handleReady();
    void _handleRun();

    void _transitionTo(State next, const String& displayText = "");

    // Wires callbacks between WiFiController and Robot
    void _registerWifiCallbacks();
};
