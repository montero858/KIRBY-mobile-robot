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

// --- L298N PWM-Pins (ENA / ENB Jumper entfernen!) ---
#define ENA  18   // Motor A Geschwindigkeit
#define ENB  19   // Motor B Geschwindigkeit
 
// --- PWM-Konfiguration (ESP32 LEDC) ---
#define PWM_KANAL_A   0     // LEDC-Kanal Motor A
#define PWM_KANAL_B   1     // LEDC-Kanal Motor B
#define PWM_FREQ      1000  // Hz
#define PWM_AUFLOES   8     // Bit → Wertebereich 0–255


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
#define FAHRGESCHWINDIGKEIT   250  // Vorwärts/Rückwärts (0–255)
#define DREHGESCHWINDIGKEIT   250  // Beim Scannen/Drehen (0–255)
#define DREH_DAUER_MS         350  // Drehzeit beim Scannen (ms)
#define SCAN_PAUSE_MS         300  // Pause zwischen Messungen (ms)

#define MOTOR_OFFSET          10   // PWM-Offset um Kurvenfahrt zu vermeiden

#endif // CONFIG_H
