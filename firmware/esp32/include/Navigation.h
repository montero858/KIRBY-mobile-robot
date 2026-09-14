// ============================================================
//  Navigation.h
//  Reactive driving logic: sensor evaluation, speed control
//  and avoidance routines.
//
//  Navigation knows neither Display nor Robot – feedback is
//  provided exclusively via callbacks:
//    DisplayCallback   – status text to the display
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
    using StuckCallback   = std::function<void()>;   // called on timeout in BLOCKED_SCAN_TURN

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
    //  Driving parameters (changeable via web UI)
    // --------------------------------------------------------
    void setMaxSpeed(uint8_t v) {
        _maxSpeed = constrain(v, 0, 255);
        if (_maxSpeed != _lastMaxSpeed) {
            _lastMaxSpeed = _maxSpeed;
            _updateLimitsBasedOnSpeed(_maxSpeed);
        }
    }

    // setTurnFrontThreshold()/setStopThreshold() make sure that
    // _turnFrontThreshold > _stopThreshold always holds, regardless of
    // the order in which the two values are set (e.g. via the web UI).
    // Otherwise _calcSpeed() would compute incorrect speeds due to a
    // negative or zero "range".
    void    setTurnFrontThreshold(int     v) { _turnFrontThreshold = max(v, _stopThreshold + 1); }
    void    setStopThreshold(int     v) {
        _stopThreshold = v;
        if (_turnFrontThreshold <= _stopThreshold) {
            _turnFrontThreshold = _stopThreshold + 1;
        }
    }
    void    setSideTurnThreshold(int v)     { _turnSideThreshold = constrain(v, 1, 400); }
    void    setTurnFactor(uint8_t v)        { _turnFactor = constrain(v, 0, 100); }

    uint8_t maxSpeed()      const { return _maxSpeed;      }
    int     turnFrontThreshold() const { return _turnFrontThreshold; }
    int     stopThreshold() const { return _stopThreshold; }
    int     sideTurnThreshold() const { return _turnSideThreshold; }
    uint8_t turnFactor()        const { return _turnFactor; }


    // --------------------------------------------------------
    //  Sensor values (for the /status web endpoint)
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
    uint8_t _lastMaxSpeed = 0;   // fuer setMaxSpeed(): erkennt echte Aenderungen,
                             // damit _updateLimitsBasedOnSpeed() nur bei
                             // Bedarf laeuft, statt bei jedem _calcSpeed()-Aufruf
    uint16_t _reverseDuration;
    uint16_t _safetyDelay;
    uint16_t _scanTurnTimeout;

    int _turnFrontThreshold;
    int _turnSideThreshold;
    int _stopThreshold;
    int _margin;
    int _innerSpeed;  // speed of the inner wheel during a turn

    float _turnFactor;


    // ---------------------------------------------------------
    // Timing
    // ---------------------------------------------------------

    unsigned long _stateStartedAt = 0;
    unsigned long _lastSensorUpdate = 0;

    // Set to false when lifted (floor == true) and only armed once the
    // floor is detected again afterwards. Only then does the
    // _safetyDelay timer start running (see _updateEmergencyStop()) -
    // this way the robot always waits the full delay AFTER touching
    // down again, regardless of how long it was in the air before.
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

    void _updateLimitsBasedOnSpeed(uint8_t speed);

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
