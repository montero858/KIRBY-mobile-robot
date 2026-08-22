// ============================================================
//  UltrasonicSensor.cpp
// ============================================================

#include "UltrasonicSensor.h"

// ============================================================
//  Constructor
// ============================================================

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin,
                                   int maxDistCm)
    : _trigPin(trigPin),
      _echoPin(echoPin),
      _maxDistCm(maxDistCm),
      _lastDistance(INVALID_DISTANCE),
      _timeoutUs((uint32_t)maxDistCm * 58UL)
{}

// ============================================================
//  begin()
// ============================================================

void UltrasonicSensor::begin() {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    digitalWrite(_trigPin, LOW);
}

// ============================================================
//  measureDistance()
//  Sends a 10µs TRIG pulse and measures the ECHO travel time.
//  Conversion: distance [cm] = travel time [µs] / 58
// ============================================================

int UltrasonicSensor::measureDistance() {
    // TRIG: make sure it was LOW before, then 10µs HIGH
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    long duration = pulseIn(_echoPin, HIGH, _timeoutUs);

    if (duration == 0) {
        // Timeout: no echo received (obstacle too far or no object)
        _lastDistance = INVALID_DISTANCE;
        return INVALID_DISTANCE;
    }

    int distance = (int)(duration / 58L);

    if (distance < 1 || distance > _maxDistCm) {
        _lastDistance = INVALID_DISTANCE;
        return INVALID_DISTANCE;
    }

    _lastDistance = distance;
    return distance;
}

// ============================================================
//  measureAverage()
//  Performs N individual measurements and returns the average
//  of the valid readings. Invalid readings are skipped.
// ============================================================

int UltrasonicSensor::measureAverage(uint8_t samples,
                                      uint16_t sampleDelayMs) {
    long   sum   = 0;
    uint8_t valid = 0;

    for (uint8_t i = 0; i < samples; i++) {
        int d = measureDistance();
        if (d != INVALID_DISTANCE) {
            sum += d;
            valid++;
        }
        if (i < samples - 1) {
            delay(sampleDelayMs);
        }
    }

    if (valid == 0) {
        _lastDistance = INVALID_DISTANCE;
        return INVALID_DISTANCE;
    }

    _lastDistance = (int)(sum / valid);
    return _lastDistance;
}
