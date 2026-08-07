// ============================================================
//  Robot.h
//  Oberste Schicht: Zustandsmaschine und Verdrahtung aller
//  Subsysteme.
//
//  Zustaende:
//    INIT   – Hardware wird initialisiert (begin())
//    READY  – Bereit, wartet auf Start-Befehl (Motoren aus)
//    RUN    – Autonome Fahrt aktiv (Navigation::update())
//    STOP   – Notfall-Stopp, wartet auf reset()
//
//  Erlaubte Uebergaenge:
//    INIT  → READY  (automatisch nach begin())
//    READY → RUN    (start() / Web /start)
//    RUN   → READY  (stop()  / Web /stopp)
//    STOP  → READY  (reset())
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
    enum class State { INIT, READY, RUN, STOP };

    Robot();

    // In Arduino setup() aufrufen.
    // Initialisiert alle Subsysteme, geht danach nach READY.
    void begin();

    // In Arduino loop() aufrufen.
    void update();

    // --------------------------------------------------------
    //  Zustandsuebergaenge
    // --------------------------------------------------------
    void start();         // READY → RUN
    void stop();          // RUN   → READY
    void reset();         // STOP  → READY

    State state() const { return _state; }

    // Navigation fuer Webserver-Callbacks oeffentlich zugaenglich
    Navigation& navigation() { return _navigation; }

private:
    State _state;

    // --------------------------------------------------------
    //  Subsysteme
    // --------------------------------------------------------
    MotorDriver      _motor;
    UltrasonicSensor _sonarCenter;
    UltrasonicSensor _sonarLeft;
    UltrasonicSensor _sonarRight;
    IrSensor         _irFloorLeft;
    IrSensor         _irFloorRight;
    IrSensor         _irFloorBottom;   // hoechste Prioritaet – Notstopp
    Display          _display;
    Navigation       _navigation;
    WiFiController   _wifi;

    // --------------------------------------------------------
    //  Zustandshandler (je ein Handler pro State)
    // --------------------------------------------------------
    void _handleInit();
    void _handleReady();
    void _handleRun();
    void _handleStop();

    void _transitionTo(State next, const String& displayText = "");

    // Verdrahtet Callbacks zwischen WiFiController und Robot
    void _registerWifiCallbacks();
};