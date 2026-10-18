// ============================================================
//  Config.h – Central hardware and behavior configuration
//
//  All pins, thresholds and driving parameters are bundled
//  here. Only Robot.cpp and the respective driver constructors
//  read these values – all other classes remain free of them.
// ============================================================

#pragma once

#include <Arduino.h>
#include "Secrets.h"   // SSID/PASSWORD – local, not version-controlled (see Secrets.example.h)

namespace Config {

    // --------------------------------------------------------
    //  Motor control (L298N) – side B (3V3)
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
    //  Ultrasonic sensors HC-SR04 – side A (VIN)
    //  ECHO pins via logic level converter (5V → 3.3V)!
    // --------------------------------------------------------
    namespace Ultrasonic {
        constexpr uint8_t  TRIG_CENTER      = 27;
        constexpr uint8_t  ECHO_CENTER      = 14;
        constexpr uint8_t  TRIG_LEFT        = 26;
        constexpr uint8_t  ECHO_LEFT        = 25;
        constexpr uint8_t  TRIG_RIGHT       = 33;
        constexpr uint8_t  ECHO_RIGHT       = 32;
        constexpr int      MAX_DISTANCE_CM  = 200;
        constexpr uint16_t SAMPLE_DELAY_MS  = 20;
    }

    // --------------------------------------------------------
    //  IR sensor TCRT5000 – side B (3V3)
    // --------------------------------------------------------
    namespace Ir {
        constexpr uint8_t  IR_FLOOR          = 13;
        constexpr uint8_t  REFLECTION_LEVEL  = LOW;
        constexpr uint16_t DEBOUNCE_US       = 500;
    }

    // --------------------------------------------------------
    //  Display SSD1306 (I2C) – side B (3V3)
    // --------------------------------------------------------
    namespace DisplayCfg {
        constexpr uint8_t SDA      = 21;
        constexpr uint8_t SCL      = 22;
        constexpr uint8_t I2C_ADDR = 0x3C;
        constexpr uint8_t WIDTH    = 128;
        constexpr uint8_t HEIGHT   = 64;
    }

    // --------------------------------------------------------
    //  Driving behavior
    //  Namespace is called NavigationCfg to avoid collision
    //  with the Navigation class.
    // --------------------------------------------------------
    namespace NavigationCfg {
        constexpr uint8_t FRONT_TURN_THRSHLD_SPD1_CM = 50; 
        constexpr uint8_t FRONT_TURN_THRSHLD_SPD2_CM = 65;
        constexpr uint8_t FRONT_TURN_THRSHLD_SPD3_CM = 70;
        
        constexpr uint8_t SIDE_TURN_THRSHLD_SPD1_CM  = 40;
        constexpr uint8_t SIDE_TURN_THRSHLD_SPD2_CM  = 45;
        constexpr uint8_t SIDE_TURN_THRSHLD_SPD3_CM  = 45;

        constexpr uint8_t STOP_THRSHLD_SPD1_CM       = 15;
        constexpr uint8_t STOP_THRSHLD_SPD2_CM       = 17; 
        constexpr uint8_t STOP_THRSHLD_SPD3_CM       = 18;
        
        constexpr uint8_t MARGIN_CM                  = 20;
        
        constexpr int8_t TURN_FACTOR_SPD1            = 10; 
        constexpr int8_t TURN_FACTOR_SPD2            = 5; 
        constexpr int8_t TURN_FACTOR_SPD3            = 1;  

        constexpr uint8_t SPEED_LEVEL_1              = 165; 
        constexpr uint8_t SPEED_LEVEL_2              = 205;
        constexpr uint8_t SPEED_LEVEL_3              = 245;
        
        constexpr uint16_t REV_DURATION_SPD1_MS      = 400; 
        constexpr uint16_t REV_DURATION_SPD2_MS      = 350; 
        constexpr uint16_t REV_DURATION_SPD3_MS      = 300;

        constexpr bool     EMERGENCY_STOP_ENABLED    = false;  // set to true to enable emergency stop when lifted
        constexpr uint16_t SAFETY_DELAY_MS           = 2000;
        constexpr uint16_t SCAN_TURN_TIMEOUT_MS      = 5000;   // max. turn time before backing up again
    }

    // --------------------------------------------------------
    //  WiFi access point
    // --------------------------------------------------------
    namespace Wifi {
        // SSID and PASSWORD come from Secrets.h (see include above)
        constexpr uint8_t CHANNEL     = 1;
        constexpr bool    HIDDEN      = false;
        constexpr uint8_t MAX_CLIENTS = 1;
    }

} // namespace Config
