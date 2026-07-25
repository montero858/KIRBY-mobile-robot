// ============================================================
//  MotorDriver.h
//  Abstraktion für den L298N Dual-H-Bridge Motortreiber
//
//  Verwaltet zwei Motorkanäle (A = links, B = rechts) über
//  ESP32 LEDC-PWM. Positiver Geschwindigkeitswert = vorwärts,
//  negativer Wert = rückwärts, 0 = Stopp.
//
//  Zustandscaching: Motorregister werden nur dann neu
//  beschrieben, wenn sich der Sollwert tatsächlich geändert
//  hat (vermeidet redundante GPIO-Writes in jedem Loop).
// ============================================================

#pragma once

#include <Arduino.h>

class MotorDriver {
public:
    // --------------------------------------------------------
    //  Konstruktor
    //  in1/in2  : Richtungspins Kanal A (links)
    //  in3/in4  : Richtungspins Kanal B (rechts)
    //  ena      : PWM-Pin Kanal A
    //  enb      : PWM-Pin Kanal B
    //  chA/chB  : LEDC-Kanal-Nummern (0–15, je eindeutig)
    //  freq     : PWM-Frequenz in Hz (z.B. 25000 = 25 kHz)
    //  resolution: PWM-Bittiefe (8 → Wertebereich 0–255)
    // --------------------------------------------------------
    MotorDriver(uint8_t in1, uint8_t in2,
                uint8_t in3, uint8_t in4,
                uint8_t ena, uint8_t enb,
                uint8_t chA, uint8_t chB,
                uint32_t freq     = 25000,
                uint8_t resolution = 8);

    // Initialisierung: Pins konfigurieren, LEDC einrichten
    void begin();

    // --------------------------------------------------------
    //  Basissteuerung
    //  speedA, speedB: -255 bis +255
    //    positiv = vorwärts
    //    negativ = rückwärts
    //    0       = Stopp (freilaufend)
    // --------------------------------------------------------
    void setMotors(int speedA, int speedB);

    // --------------------------------------------------------
    //  Komfortmethoden
    // --------------------------------------------------------
    void stop();
    void forward(uint8_t speed);
    void backward(uint8_t speed);

    // Pivotdrehung auf der Stelle (ein Rad vor, eines zurück)
    void turnLeft(uint8_t speed);
    void turnRight(uint8_t speed);

    // Kurvenfahrt (Außenrad schneller als Innenrad)
    // outerSpeed: Geschwindigkeit des Außenrads (0–255)
    // innerSpeed: Geschwindigkeit des Innenrads (0–255)
    void curveLeft(uint8_t outerSpeed, uint8_t innerSpeed);
    void curveRight(uint8_t outerSpeed, uint8_t innerSpeed);

    // Aktuelle Sollwerte abfragen
    int getSpeedA() const { return _lastSpeedA; }
    int getSpeedB() const { return _lastSpeedB; }

private:
    // Einen einzelnen Kanal setzen (intern)
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
