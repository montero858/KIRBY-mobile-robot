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
    using DisplayCallback = std::function<void(const String&)>;
    using StuckCallback   = std::function<void()>;   // wird bei Timeout in BLOCKED_SCAN_TURN aufgerufen

    // ---------------------------------------------------------
    // Navigation states
    // ---------------------------------------------------------

    enum class State : uint8_t
    {
        DRIVING,

        BLOCKED_REVERSE,
        BLOCKED_SCAN_TURN,
        EMERGENCY_STOP
    };


    // ---------------------------------------------------------
    // Sensor snapshot
    // ---------------------------------------------------------

    struct SensorSnapshot
    {
        int center = 999;
        int left   = 999;
        int right  = 999;

        bool floor = false;
    };


    // ---------------------------------------------------------
    // Constructor
    // ---------------------------------------------------------

    Navigation(
        MotorDriver& motor,
        UltrasonicSensor& sonarCenter,
        UltrasonicSensor& sonarLeft,
        UltrasonicSensor& sonarRight,
        IrSensor& irFloor
    );

    void setDisplayCallback  (DisplayCallback   cb) { _displayCb   = cb; }
    void setStuckCallback  (StuckCallback   cb) { _stuckCb   = cb; }

    // ---------------------------------------------------------
    // Main control
    // ---------------------------------------------------------

    /**
     * Must be called continuously from Robot::update().
     *
     * The method is deliberately non-blocking:
     * no delay() based navigation logic is used here.
     */
    void update();


    /**
     * Start autonomous navigation.
     */
    void start();


    /**
     * Stop autonomous navigation immediately.
     */
    void stop();


    // ---------------------------------------------------------
    // State information
    // ---------------------------------------------------------

    State state() const;

    const SensorSnapshot& sensors() const;


    // --------------------------------------------------------
    //  Fahrparameter (per Web-UI aenderbar)
    // --------------------------------------------------------
    void    setMaxSpeed     (uint8_t v) { _maxSpeed      = constrain(v, 0, 255); }

    // setTurnFrontThreshold()/setStopThreshold() stellen sicher, dass
    // _turnFrontThreshold > _stopThreshold bleibt, unabhaengig von der
    // Reihenfolge, in der beide Werte (z.B. per Web-UI) gesetzt werden.
    // Andernfalls wuerde _calcSpeed() durch ein negatives oder Null-
    // "range" fehlerhafte Geschwindigkeiten berechnen.
    void    setTurnFrontThreshold(int     v) { _turnFrontThreshold = max(v, _stopThreshold + 1); }
    void    setStopThreshold(int     v) {
        _stopThreshold = v;
        if (_turnFrontThreshold <= _stopThreshold) {
            _turnFrontThreshold = _stopThreshold + 1;
        }
    }

    uint8_t maxSpeed()      const { return _maxSpeed;      }
    int     turnFrontThreshold() const { return _turnFrontThreshold; }
    int     stopThreshold() const { return _stopThreshold; }


    // --------------------------------------------------------
    //  Sensorwerte (fuer Webendpoint /status)
    // --------------------------------------------------------
    
    int  distanceCenter();
    int  distanceLeft();
    int  distanceRight();
    bool floor();

private:

    // ---------------------------------------------------------
    // Hardware references
    // ---------------------------------------------------------

    MotorDriver& _motor;

    UltrasonicSensor& _sonarCenter;
    UltrasonicSensor& _sonarLeft;
    UltrasonicSensor& _sonarRight;

    IrSensor& _irFloor;

    DisplayCallback   _displayCb;
    StuckCallback     _stuckCb;

    // ---------------------------------------------------------
    // Sensor state
    // ---------------------------------------------------------

    SensorSnapshot _sensors;


    // ---------------------------------------------------------
    // Navigation state
    // ---------------------------------------------------------

    State _state = State::DRIVING;



    // ---------------------------------------------------------
    // Runtime parameters
    // ---------------------------------------------------------

    uint8_t _maxSpeed;
    uint16_t _reverseDuration;
    uint16_t _safetyDelay;
    uint16_t _scanTurnTimeout;

    int _turnFrontThreshold;
    int _stopThreshold;
    int _margin;


    // ---------------------------------------------------------
    // Timing
    // ---------------------------------------------------------

    unsigned long _stateStartedAt = 0;
    unsigned long _lastSensorUpdate = 0;

    // Wird beim Anheben (floor == true) auf false gesetzt und erst
    // scharf geschaltet, sobald der Boden danach wieder erkannt wird.
    // Erst dann beginnt der _safetyDelay-Timer zu laufen (siehe
    // _updateEmergencyStop()) - so wartet der Roboter immer die volle
    // Verzoegerung NACH dem Wiederaufsetzen, unabhaengig davon wie
    // lange er vorher in der Luft war.
    bool _emergencyRecoveryArmed = false;


    // ---------------------------------------------------------
    // Sensor handling
    // ---------------------------------------------------------

    /**
     * Read all navigation sensors and update _sensors.
     */
    void _updateSensors();


    // ---------------------------------------------------------
    // State handlers
    // ---------------------------------------------------------

    void _updateDriving();
    void _updateBlockedReverse();
    void _updateBlockedScanAndTurn();
    void _updateEmergencyStop();


    // ---------------------------------------------------------
    // Avoidance
    // ---------------------------------------------------------

    /**
     * Start the normal obstacle avoidance sequence.
     */
    void _startBlockedReverse();
    void _startBlockedScanAndTurn();

   
    // ---------------------------------------------------------
    // Motion
    // ---------------------------------------------------------

    /**
     * Calculate a speed depending on the distance
     * to the obstacle.
     */
    uint8_t _calcSpeed(int distanceCm) const;


    // ---------------------------------------------------------
    // State / timing helpers
    // ---------------------------------------------------------

    /**
     * Change state and reset the state timer.
     */
    void _transitionTo(State next);


    /**
     * Check whether the current state has been active
     * for at least the specified amount of time.
     */
    bool _stateElapsed(unsigned long duration) const;

    void _display(const String& text);
};