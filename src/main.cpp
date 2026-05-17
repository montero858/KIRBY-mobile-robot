#include <Arduino.h>
#include "SR04.h"

// ============================================================
//  Selbstfahrender Roboter – OHNE PWM (Jumper auf ENA/ENB)
//  Hardware: ESP32 + L298N + HC-SR04 + 2x DC-Motor
//  ENA und ENB Jumper auf L298N aufgesteckt lassen!
// ============================================================

// --- Pin-Definitionen ---
// L298N Steuerpins (nur Richtung, kein PWM)
const int IN1 = 26;   // Motor A Richtung
const int IN2 = 27;   // Motor A Richtung
const int IN3 = 14;   // Motor B Richtung
const int IN4 = 12;   // Motor B Richtung
// ENA und ENB werden NICHT verwendet – Jumper übernehmen das!

// HC-SR04 Ultraschallsensor
#define TRIG_PIN 5
#define ECHO_PIN 4

SR04 sr04 = SR04(ECHO_PIN, TRIG_PIN);

long a;

// --- Einstellungen ---
const int HINDERNIS_ABSTAND_CM = 25;  // Stopp wenn < 25 cm
const int DREH_DAUER_MS        = 500; // Wie weit rechts/links gedreht wird
const int SCAN_PAUSE_MS        = 300; // Pause zwischen Messungen

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
  Serial.println("  → Scanne RECHTS...");
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
  Serial.println("  → Scanne LINKS...");
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
  Serial.println("=== Roboter startet (ohne PWM) ===");

  // Nur Richtungspins nötig – kein ledcSetup!
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  motorenStopp();
  delay(2000);
  Serial.println("=== Los! ===");
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
    vorwaerts();

  } else {
    // Hindernis erkannt!
    Serial.println("⚠ Hindernis! Stoppe...");
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
