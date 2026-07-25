// ============================================================
//  Navigation.h
//  Reaktive Fahrlogik: Sensorauswertung, Geschwindigkeits-
//  regelung und Ausweichroutinen.
//
//  Navigation kennt weder Display noch Robot – Statusmeldungen
//  werden ueber einen Display-Callback nach aussen gegeben.
//  Alle Fahrparameter sind zur Laufzeit aenderbar (Web-UI).
// ============================================================

#pragma once

#include <Arduino.h>
#include <functional>
#include "MotorDriver.h"
#include "UltrasonicSensor.h"
#include "IrSensor.h"

class Navigation {
public:
    using DisplayCallback = std::function<void(const String&)>;

    // --------------------------------------------------------
    //  Konstruktor
    //  Alle Sensor- und Motorobjekte werden per Referenz
    //  uebergeben und muessen laenger leben als Navigation.
    // --------------------------------------------------------
    Navigation(MotorDriver&      motor,
               UltrasonicSensor& sonarCenter,
               UltrasonicSensor& sonarLeft,
               UltrasonicSensor& sonarRight,
               IrSensor&         irFloorLeft,
               IrSensor&         irFloorRight);

    // Optionalen Display-Callback registrieren.
    // Wird aufgerufen wann immer Navigation einen Status ausgibt.
    void setDisplayCallback(DisplayCallback cb) { _displayCb = cb; }

    // --------------------------------------------------------
    //  Einen Navigations-Zyklus ausfuehren (in loop() bzw.
    //  Robot::_handleRun() aufrufen).
    // --------------------------------------------------------
    void update();

    // Oeffentlich fuer den /scan-Webendpoint
    void scanAndTurn();

    // --------------------------------------------------------
    //  Fahrparameter (per Web-UI aenderbar)
    // --------------------------------------------------------
    void    setMaxSpeed(uint8_t v)     { _maxSpeed      = constrain(v, 0, 255); }
    void    setSlowThreshold(int v)    { _slowThreshold = v; }
    void    setStopThreshold(int v)    { _stopThreshold = v; }
    void    setTurnSpeed(uint8_t v)    { _turnSpeed     = constrain(v, 0, 255); }

    uint8_t maxSpeed()       const { return _maxSpeed;      }
    int     slowThreshold()  const { return _slowThreshold; }
    int     stopThreshold()  const { return _stopThreshold; }
    uint8_t turnSpeed()      const { return _turnSpeed;     }

    // --------------------------------------------------------
    //  Direkte Sensorwerte (fuer Webendpoint /abstand)
    // --------------------------------------------------------
    int  distanceCenter();
    int  distanceLeft();
    int  distanceRight();
    bool floorFrontLeft();
    bool floorFrontRight();

private:
    MotorDriver&      _motor;
    UltrasonicSensor& _sonarCenter;
    UltrasonicSensor& _sonarLeft;
    UltrasonicSensor& _sonarRight;
    IrSensor&         _irFloorLeft;
    IrSensor&         _irFloorRight;

    DisplayCallback _displayCb;

    uint8_t _maxSpeed;
    int     _slowThreshold;
    int     _stopThreshold;
    uint8_t _turnSpeed;

    void    _display(const String& text);
    uint8_t _calcSpeed(int distanceCm);
    void    _routineFloorFront();
};
