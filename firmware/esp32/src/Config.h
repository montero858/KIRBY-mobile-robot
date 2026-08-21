// ============================================================
//  Config.h – Zentrale Hardware- und Verhaltenskonfiguration
//
//  Alle Pins, Schwellen und Fahrtparameter sind hier gebuendelt.
//  Nur Robot.cpp und die jeweiligen Treiber-Konstruktoren
//  lesen diese Werte – alle anderen Klassen bleiben frei davon.
// ============================================================

#pragma once

#include <Arduino.h>
#include "Secrets.h"   // SSID/PASSWORD – lokal, nicht versioniert (siehe Secrets.example.h)

namespace Config {

    // --------------------------------------------------------
    //  Motorsteuerung (L298N) – Seite B (3V3)
    // --------------------------------------------------------
    namespace Motor {
        constexpr uint8_t  IN1           = 4;
        constexpr uint8_t  IN2           = 16;  // RX2
        constexpr uint8_t  IN3           = 17;  // TX2
        constexpr uint8_t  IN4           = 5;
        constexpr uint8_t  ENA           = 18;
        constexpr uint8_t  ENB           = 19;  
        constexpr uint8_t  PWM_CHANNEL_A = 0;
        constexpr uint8_t  PWM_CHANNEL_B = 1;
        constexpr uint32_t PWM_FREQ      = 2000; // 2 kHz
        constexpr uint8_t  PWM_BITS      = 8;     // 0–255
    }

    // --------------------------------------------------------
    //  Ultraschallsensoren HC-SR04 – Seite A (VIN)
    //  ECHO-Pins ueber Logic Level Converter (5V → 3,3V)!
    // --------------------------------------------------------
    namespace Ultrasonic {
        constexpr uint8_t  TRIG_CENTER      = 27;
        constexpr uint8_t  ECHO_CENTER      = 14;
        constexpr uint8_t  TRIG_LEFT        = 26;
        constexpr uint8_t  ECHO_LEFT        = 25;
        constexpr uint8_t  TRIG_RIGHT       = 33;
        constexpr uint8_t  ECHO_RIGHT       = 32;
        constexpr int      MAX_DISTANCE_CM  = 400;
        constexpr uint16_t SAMPLE_DELAY_MS  = 20;
    }

    // --------------------------------------------------------
    //  IR-Sensoren TCRT5000 – Seite A (VIN, input-only Pins)
    // --------------------------------------------------------
    namespace Ir {
        constexpr uint8_t  IR_FLOOR          = 13;
        constexpr uint8_t  REFLECTION_LEVEL  = LOW;
        constexpr uint16_t DEBOUNCE_US       = 500;
    }

    // --------------------------------------------------------
    //  Display SSD1306 (I2C) – Seite B (3V3)
    // --------------------------------------------------------
    namespace DisplayCfg {
        constexpr uint8_t SDA      = 21;
        constexpr uint8_t SCL      = 22;
        constexpr uint8_t I2C_ADDR = 0x3C;
        constexpr uint8_t WIDTH    = 128;
        constexpr uint8_t HEIGHT   = 64;
    }

    // --------------------------------------------------------
    //  Fahrverhalten
    //  Namespace heisst NavigationCfg um Kollision mit der
    //  Klasse Navigation zu vermeiden.
    // --------------------------------------------------------
    namespace NavigationCfg {
        constexpr int     FRONT_TURN_THRESHOLD_CM   = 80;
        constexpr int     SIDE_TURN_THRESHOLD_CM    = 40;
        constexpr int     STOP_THRESHOLD_CM         = 15;
        constexpr int     MARGIN_CM                 = 20;
        constexpr uint8_t SPEED_MIN                 = 0;    // Minimum für enge Kurvenfahrt
        constexpr uint8_t SPEED_SLOW                = 135;
        constexpr uint8_t SPEED_LEVEL_1             = 135;
        constexpr uint8_t SPEED_LEVEL_2             = 165;
        constexpr uint8_t SPEED_LEVEL_3             = 195;
        constexpr uint8_t SPEED_LEVEL_4             = 225;
        constexpr uint16_t REVERSE_DURATION_MS      = 500;
        constexpr uint16_t SAFETY_DELAY_MS          = 2000;
        constexpr uint16_t SCAN_TURN_TIMEOUT_MS     = 5000;   // Max. Drehzeit vor erneutem Zurücksetzen
    }

    // --------------------------------------------------------
    //  WLAN Access Point
    // --------------------------------------------------------
    namespace Wifi {
        // SSID und PASSWORD kommen aus Secrets.h (siehe Include oben)
        constexpr uint8_t CHANNEL     = 1;
        constexpr bool    HIDDEN      = false;
        constexpr uint8_t MAX_CLIENTS = 1;
    }

} // namespace Config