// ============================================================
//  UltrasonicSensor.h
//  Abstraktion für den HC-SR04 Ultraschallsensor
//
//  Misst den Abstand per Echolaufzeit direkt über TRIG/ECHO-
//  Pins (kein externes Library-Abhängigkeit).
//
//  Ungültige Messungen (Timeout, Wertebereich überschritten)
//  werden durch den Rückgabewert INVALID_DISTANCE (999)
//  signalisiert.
//
//  measureDistance()    – Einzelmessung
//  measureAverage()     – Mittelwert aus N Messungen,
//                         ungültige Einzelwerte werden
//                         automatisch verworfen
// ============================================================

#pragma once

#include <Arduino.h>

class UltrasonicSensor {
public:
    // Rückgabewert bei ungültiger Messung (Timeout / außerhalb 1–400 cm)
    static constexpr int INVALID_DISTANCE = 999;

    // --------------------------------------------------------
    //  Konstruktor
    //  trigPin  : GPIO-Pin TRIG (Output)
    //  echoPin  : GPIO-Pin ECHO (Input, über Logic Level
    //             Converter wenn 5V-Sensor am 3,3V-ESP32!)
    //  maxDistCm: Maximale gültige Messdistanz in cm (default 400)
    // --------------------------------------------------------
    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin,
                     int maxDistCm = 400);

    // Pins initialisieren
    void begin();

    // --------------------------------------------------------
    //  Einzelmessung
    //  Gibt den Abstand in cm zurück.
    //  Gibt INVALID_DISTANCE zurück bei Timeout oder wenn der
    //  Messwert außerhalb [1, maxDistCm] liegt.
    // --------------------------------------------------------
    int measureDistance();

    // --------------------------------------------------------
    //  Gemittelte Messung
    //  samples    : Anzahl der Einzelmessungen (default 2)
    //  sampleDelay: Pause zwischen Messungen in ms (default 20)
    //
    //  Ungültige Einzelmessungen werden verworfen.
    //  Sind alle Messungen ungültig, wird INVALID_DISTANCE
    //  zurückgegeben.
    // --------------------------------------------------------
    int measureAverage(uint8_t samples = 2, uint16_t sampleDelayMs = 20);

    // Letzte gemessene Distanz (ohne neue Messung)
    int getLastDistance() const { return _lastDistance; }

private:
    uint8_t _trigPin;
    uint8_t _echoPin;
    int     _maxDistCm;
    int     _lastDistance;

    // Timeout in Mikrosekunden: Schall braucht bei maxDistCm
    // (Hin- und Rückweg) maxDistCm * 2 / 0.034 µs ≈ maxDistCm * 58 µs
    uint32_t _timeoutUs;
};
