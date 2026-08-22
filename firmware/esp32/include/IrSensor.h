// ============================================================
//  IrSensor.h
//  Abstraction for TCRT5000-based IR reflectance sensors
//
//  Supports two semantically opposite operating modes:
//
//  Mode::OBSTACLE  – Alarm when reflection is detected
//                    (object nearby)
//                    Typical use: side obstacles
//
//  Mode::FLOOR     – Alarm when NO reflection is detected
//                    (no floor present = risk of falling!)
//                    Typical use: edge/cliff protection
//
//  Debouncing: isTriggered() evaluates two consecutive
//  readings. Both must agree for an alarm to be considered
//  valid. This prevents false triggers from brief noise
//  spikes.
// ============================================================

#pragma once

#include <Arduino.h>

class IrSensor {
public:
    // Operating mode
    enum class Mode {
        OBSTACLE,   // Alarm when reflection present (object nearby)
        FLOOR       // Alarm when no reflection (no floor)
    };

    // --------------------------------------------------------
    //  Constructor
    //  pin            : GPIO pin (input)
    //  mode           : OBSTACLE or FLOOR (see above)
    //  reflectionLevel: electrical level on reflection
    //                   (TCRT5000 typically: LOW)
    //  debounceUs     : delay between the two debouncing
    //                   readings in microseconds (default 500)
    // --------------------------------------------------------
    IrSensor(uint8_t pin, Mode mode,
             uint8_t reflectionLevel = LOW,
             uint16_t debounceUs = 500);

    // Initialize pin as input
    void begin();

    // --------------------------------------------------------
    //  isTriggered()  – Debounced query
    //  Returns true if the sensor reports an alarm state
    //  (meaning depends on Mode, see above).
    //  Two consecutive readings must agree.
    // --------------------------------------------------------
    bool isTriggered();

    // --------------------------------------------------------
    //  rawReflection()  – Unfiltered single reading
    //  Returns true if a reflection is detected
    //  (floor/object present), regardless of Mode.
    //  Suitable for status displays, not for safety-critical
    //  decisions.
    // --------------------------------------------------------
    bool rawReflection();

private:
    uint8_t  _pin;
    Mode     _mode;
    uint8_t  _reflectionLevel;
    uint16_t _debounceUs;
};
