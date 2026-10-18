// ============================================================
//  MotorDriver.h
//  Abstraction for the L298N dual H-bridge motor driver
//
//  Manages two motor channels (A = left, B = right) via
//  ESP32 LEDC PWM. Positive speed value = forward,
//  negative value = backward, 0 = stop.
//
//  State caching: motor registers are only rewritten when
//  the target value has actually changed (avoids redundant
//  GPIO writes on every loop).
// ============================================================

#pragma once

#include <Arduino.h>

class MotorDriver {
public:
    // --------------------------------------------------------
    //  Constructor
    //  in1/in2  : direction pins channel A (left)
    //  in3/in4  : direction pins channel B (right)
    //  ena      : PWM pin channel A
    //  enb      : PWM pin channel B
    //  chA/chB  : LEDC channel numbers (0–15, must be unique)
    //  freq     : PWM frequency in Hz (e.g. 25000 = 25 kHz)
    //  resolution: PWM bit depth (8 → value range 0–255)
    // --------------------------------------------------------
    MotorDriver(uint8_t in1, uint8_t in2,
                uint8_t in3, uint8_t in4,
                uint8_t ena, uint8_t enb,
                uint8_t chA, uint8_t chB,
                uint32_t freq     = 25000,
                uint8_t resolution = 8);

    // Initialization: configure pins, set up LEDC
    void begin();

    // --------------------------------------------------------
    //  Basic control
    //  speedA, speedB: -255 to +255
    //    positive = forward
    //    negative = backward
    //    0        = stop (coasting)
    // --------------------------------------------------------
    void setMotors(int speedA, int speedB);

    // --------------------------------------------------------
    //  Convenience methods
    // --------------------------------------------------------
    void stop();
    void forward(uint8_t speed);
    void backward(uint8_t speed);

    // Pivot turn in place (one wheel forward, one backward)
    void turnLeft(uint8_t speed);
    void turnRight(uint8_t speed);

    // Curved drive (outer wheel faster than inner wheel)
    // outerSpeed: speed of the outer wheel (-255 to +255)
    // innerSpeed: speed of the inner wheel (-255 to +255)
    //   Normally positive (slower than outer); negative reverses the
    //   inner wheel for a tighter turn.
    void curveLeft(int16_t outerSpeed, int16_t innerSpeed);
    void curveRight(int16_t outerSpeed, int16_t innerSpeed);

    // Query current target values
    int getSpeedA() const { return _lastSpeedA; }
    int getSpeedB() const { return _lastSpeedB; }

private:
    // Set a single channel (internal)
    void _setChannel(uint8_t in1, uint8_t in2,
                     uint8_t pwmChannel, int speed);

    uint8_t  _in1, _in2, _in3, _in4;
    uint8_t  _ena, _enb;
    uint8_t  _chA, _chB;
    uint32_t _freq;
    uint8_t  _resolution;

    int _lastSpeedA;
    int _lastSpeedB;
};