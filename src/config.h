// ============================================================
//  config.h – Pin-Belegung und Einstellungen
//  Hier alle hardware-spezifischen Werte zentral anpassen
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// --- L298N Richtungspins ---
#define IN1  26   // Motor A Richtung
#define IN2  27   // Motor A Richtung
#define IN3  14   // Motor B Richtung
#define IN4  12   // Motor B Richtung
// ENA / ENB: Jumper auf L298N gesetzt, keine Pin-Definition nötig

// --- HC-SR04 Ultraschallsensor ---
#define TRIG  5
#define ECHO  4

// --- Display (SSD1306 I2C) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Define OLED I2C address (default is usually 0x3C or 0x3D)
#define OLED_ADDR   0x3C
#define PIN_SCL     21
#define PIN_SDA     22


// --- Fahrverhalten ---
#define HINDERNIS_ABSTAND_CM  25   // Stopp wenn < 25 cm
#define DREH_DAUER_MS         350  // Drehzeit beim Scannen (ms)
#define SCAN_PAUSE_MS         300  // Pause zwischen Messungen (ms)

#endif // CONFIG_H
