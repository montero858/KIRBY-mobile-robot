// ============================================================
//  Selbstfahrender Roboter – OHNE PWM (Jumper auf ENA/ENB)
//  Hardware: ESP32 + L298N + HC-SR04 + 2x DC-Motor
//  ENA und ENB Jumper auf L298N aufgesteckt lassen!
// ============================================================

// Pin-Belegung und Einstellungen
#include "config.h"

// Für Ultrasonic Sensor
#include "SR04.h"   

// Für Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

SR04 sr04 = SR04(ECHO, TRIG);
long a;

#define OLED_RESET -1
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ============================================================
//  Display-Anzeige 
// ============================================================

void displayShow(String text) {
  Serial.println(text);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print(text.c_str());
  display.display();
}

// ============================================================
//  Motorsteuerung – nur Richtung, kein PWM
// ============================================================

void motorenStopp() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void vorwaerts() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void rueckwaerts() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

// Rechtsdrehung: Motor A vorwärts, Motor B rückwärts
void dreheRechts() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

// Linksdrehung: Motor A rückwärts, Motor B vorwärts
void dreheLinks() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// ============================================================
//  Scan: Rechts und Links messen, beste Richtung wählen
//  Gibt 'R', 'L' oder 'U' (umkehren) zurück
// ============================================================

char scanneBesteDrehrichtung() {
  // Nach rechts drehen und messen
  displayShow("Scanne rechts...");
  dreheRechts();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);

  int abstandRechts = sr04.Distance();
  Serial.print("    Abstand rechts: ");
  Serial.print(abstandRechts);
  Serial.println(" cm");

  // Zurück zur Mitte
  dreheLinks();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);

  // Nach links drehen und messen
  displayShow("Scanne links...");
  dreheLinks();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);

  int abstandLinks = sr04.Distance();
  Serial.print("    Abstand links: ");
  Serial.print(abstandLinks);
  Serial.println(" cm");

  // Zurück zur Mitte
  dreheRechts();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);

  // Entscheidung
  if (abstandLinks < HINDERNIS_ABSTAND_CM && abstandRechts < HINDERNIS_ABSTAND_CM) {
    Serial.println("  ⚠ Beide Seiten blockiert → Umkehren");
    return 'U';
  }
  if (abstandLinks >= abstandRechts) {
    Serial.println("  ✓ Links freier → drehe links");
    return 'L';
  } else {
    Serial.println("  ✓ Rechts freier → drehe rechts");
    return 'R';
  }
}

// ============================================================
//  Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL); // SDA=21, SCL=22
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  displayShow("Initialisierung");

  // Nur Richtungspins nötig – kein ledcSetup!
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  motorenStopp();
  delay(2000);
}

// ============================================================
//  Hauptschleife
// ============================================================

void loop() {
  int abstand = sr04.Distance();

  Serial.print("Abstand: ");
  Serial.print(abstand);
  Serial.println(" cm");

  if (abstand > HINDERNIS_ABSTAND_CM || abstand == 999) {
    // Freie Bahn → vorwärts
    displayShow("Fahre los");
    vorwaerts();

  } else {
    // Hindernis erkannt!
    displayShow("Hindernis erkannt!");
    motorenStopp();
    delay(400);

    // Beste Richtung scannen
    char richtung = scanneBesteDrehrichtung();

    // In die bessere Richtung drehen
    if (richtung == 'R') {
      dreheRechts();
      delay(DREH_DAUER_MS);
    } else if (richtung == 'L') {
      dreheLinks();
      delay(DREH_DAUER_MS);
    } else {
      // Umkehren: kurz rückwärts, dann ~180° drehen
      rueckwaerts();
      delay(600);
      motorenStopp();
      delay(200);
      dreheRechts();
      delay(DREH_DAUER_MS * 2);
    }

    motorenStopp();
    delay(300);
  }
}
