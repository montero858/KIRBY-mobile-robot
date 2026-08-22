// ============================================================
//  UltrasonicSensor.h
//  Abstraction for the HC-SR04 ultrasonic distance sensor
//
//  Measures distance via echo travel time directly through
//  the TRIG/ECHO pins (no external library dependency).
//
//  Invalid measurements (timeout, out of range) are signaled
//  by the return value INVALID_DISTANCE (999).
//
//  measureDistance()    – single measurement
//  measureAverage()     – average of N measurements,
//                         invalid single readings are
//                         discarded automatically
// ============================================================

#pragma once

#include <Arduino.h>

class UltrasonicSensor {
public:
    // Return value for an invalid measurement (timeout / outside 1–400 cm)
    static constexpr int INVALID_DISTANCE = 999;

    // --------------------------------------------------------
    //  Constructor
    //  trigPin  : GPIO pin TRIG (output)
    //  echoPin  : GPIO pin ECHO (input, via logic level
    //             converter if using a 5V sensor on a 3.3V ESP32!)
    //  maxDistCm: maximum valid measurement distance in cm (default 400)
    // --------------------------------------------------------
    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin,
                     int maxDistCm = 400);

    // Initialize pins
    void begin();

    // --------------------------------------------------------
    //  Single measurement
    //  Returns the distance in cm.
    //  Returns INVALID_DISTANCE on timeout or if the reading
    //  is outside [1, maxDistCm].
    // --------------------------------------------------------
    int measureDistance();

    // --------------------------------------------------------
    //  Averaged measurement
    //  samples    : number of individual measurements (default 2)
    //  sampleDelay: pause between measurements in ms (default 20)
    //
    //  Invalid individual measurements are discarded.
    //  If all measurements are invalid, INVALID_DISTANCE is
    //  returned.
    // --------------------------------------------------------
    int measureAverage(uint8_t samples = 2, uint16_t sampleDelayMs = 20);

    // Last measured distance (without taking a new measurement)
    int getLastDistance() const { return _lastDistance; }

private:
    uint8_t _trigPin;
    uint8_t _echoPin;
    int     _maxDistCm;
    int     _lastDistance;

    // Timeout in microseconds: sound needs approximately
    // maxDistCm * 2 / 0.034 µs ≈ maxDistCm * 58 µs
    // for the round trip at maxDistCm
    uint32_t _timeoutUs;
};
