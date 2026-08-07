// ============================================================
//  Navigation.h
//  Reaktive Fahrlogik: Sensorauswertung, Geschwindigkeits-
//  regelung und Ausweichroutinen.
//
//  Navigation kennt weder Display noch Robot – Rueckmeldungen
//  erfolgen ausschliesslich ueber Callbacks:
//    DisplayCallback   – Statustext ans Display
// ============================================================

#pragma once

#include <Arduino.h>
#include <functional>
#include "MotorDriver.h"
#include "UltrasonicSensor.h"
#include "IrSensor.h"

class Navigation {
public:
    using DisplayCallback   = std::function<void(const String&)>;

    // --------------------------------------------------------
    //  Konstruktor
    //  irFloorBottom: nach unten zeigender Sensor mit hoechster
    //  Prioritaet – loest sofortigen Notstopp aus.
    // --------------------------------------------------------
    Navigation(MotorDriver&      motor,
               UltrasonicSensor& sonarCenter,
               UltrasonicSensor& sonarLeft,
               UltrasonicSensor& sonarRight,
               IrSensor&         irFloorLeft,
               IrSensor&         irFloorRight,
               IrSensor&         irFloorBottom);

    void setDisplayCallback  (DisplayCallback   cb) { _displayCb   = cb; }

    // --------------------------------------------------------
    //  Einen Navigations-Zyklus ausfuehren
    // --------------------------------------------------------
    void update();

    // Oeffentlich fuer den /scan-Webendpoint
    void scanAndTurn();

    // --------------------------------------------------------
    //  Fahrparameter (per Web-UI aenderbar)
    // --------------------------------------------------------
    void    setMaxSpeed     (uint8_t v) { _maxSpeed      = constrain(v, 0, 255); }
    void    setTurnFrontThreshold(int     v) { _turnFrontThreshold = v; }
    void    setStopThreshold(int     v) { _stopThreshold = v; }
    void    setTurnSpeed    (uint8_t v) { _turnSpeed     = constrain(v, 0, 255); }

    uint8_t maxSpeed()      const { return _maxSpeed;      }
    int     turnFrontThreshold() const { return _turnFrontThreshold; }
    int     stopThreshold() const { return _stopThreshold; }
    uint8_t turnSpeed()     const { return _turnSpeed;     }

    // --------------------------------------------------------
    //  Sensorwerte (fuer Webendpoint /status)
    // --------------------------------------------------------
    int  distanceCenter();
    int  distanceLeft();
    int  distanceRight();
    bool floorFrontLeft();
    bool floorFrontRight();
    bool floorBottom();

private:
    MotorDriver&      _motor;
    UltrasonicSensor& _sonarCenter;
    UltrasonicSensor& _sonarLeft;
    UltrasonicSensor& _sonarRight;
    IrSensor&         _irFloorLeft;
    IrSensor&         _irFloorRight;
    IrSensor&         _irFloorBottom;

    DisplayCallback   _displayCb;

    uint8_t _maxSpeed;
    int     _turnFrontThreshold;
    int     _turnSideThreshold;
    int     _stopThreshold;
    uint8_t _turnSpeed;
    bool    _emergencyStopTriggered;

    void    _display(const String& text);
    uint8_t _calcSpeed(int distanceCm);
    void    _routineFloorFront(bool fallL, bool fallR);
};