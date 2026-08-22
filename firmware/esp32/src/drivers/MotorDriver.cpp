// ============================================================
//  MotorDriver.cpp
// ============================================================

#include "MotorDriver.h"

// ============================================================
//  Constructor
// ============================================================

MotorDriver::MotorDriver(uint8_t in1, uint8_t in2,
                         uint8_t in3, uint8_t in4,
                         uint8_t ena, uint8_t enb,
                         uint8_t chA, uint8_t chB,
                         uint32_t freq, uint8_t resolution)
    : _in1(in1), _in2(in2), _in3(in3), _in4(in4),
      _ena(ena), _enb(enb),
      _chA(chA), _chB(chB),
      _freq(freq), _resolution(resolution),
      _lastSpeedA(0), _lastSpeedB(0)
{}

// ============================================================
//  begin()
// ============================================================

void MotorDriver::begin() {
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    pinMode(_in3, OUTPUT);
    pinMode(_in4, OUTPUT);

    ledcSetup(_chA, _freq, _resolution);
    ledcSetup(_chB, _freq, _resolution);
    ledcAttachPin(_ena, _chA);
    ledcAttachPin(_enb, _chB);

    stop();
}

// ============================================================
//  Internal channel setter
//  speed: -255 to +255
// ============================================================

void MotorDriver::_setChannel(uint8_t in1, uint8_t in2,
                               uint8_t pwmChannel, int speed) {
    speed = constrain(speed, -255, 255);

    if (speed > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(pwmChannel, (uint8_t)speed);
    } else if (speed < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(pwmChannel, (uint8_t)(-speed));
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(pwmChannel, 0);
    }
}

// ============================================================
//  setMotors()
//  Main method: sets both channels with state caching.
//  Only writes GPIO/LEDC when a value has actually changed.
// ============================================================

void MotorDriver::setMotors(int speedA, int speedB) {
    speedA = constrain(speedA, -255, 255);
    speedB = constrain(speedB, -255, 255);

    if (speedA != _lastSpeedA) {
        _setChannel(_in1, _in2, _chA, speedA);
        _lastSpeedA = speedA;
    }
    if (speedB != _lastSpeedB) {
        _setChannel(_in3, _in4, _chB, speedB);
        _lastSpeedB = speedB;
    }
}

// ============================================================
//  Convenience methods
// ============================================================

void MotorDriver::stop() {
    setMotors(0, 0);
}

void MotorDriver::forward(uint8_t speed) {
    setMotors((int)speed, (int)speed);
}

void MotorDriver::backward(uint8_t speed) {
    setMotors(-(int)speed, -(int)speed);
}

void MotorDriver::turnLeft(uint8_t speed) {
    // Pivot turn left: left wheel backward, right wheel forward
    setMotors(-(int)speed, (int)speed);
}

void MotorDriver::turnRight(uint8_t speed) {
    // Pivot turn right: left wheel forward, right wheel backward
    setMotors((int)speed, -(int)speed);
}

void MotorDriver::curveLeft(uint8_t outerSpeed, uint8_t innerSpeed) {
    // Curve left: right wheel outer (fast), left wheel inner (slow)
    setMotors((int)innerSpeed, (int)outerSpeed);
}

void MotorDriver::curveRight(uint8_t outerSpeed, uint8_t innerSpeed) {
    // Curve right: left wheel outer (fast), right wheel inner (slow)
    setMotors((int)outerSpeed, (int)innerSpeed);
}
