// ============================================================
//  Selbstfahrender Roboter – PWM + WLAN-Steuerung
//  Hardware: ESP32 + L298N + HC-SR04 + 2x DC-Motor
//  Steuerung: Browser unter http://192.168.4.1
// ============================================================

// Pin-Belegung und Einstellungen
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include "WiFiControl.h"

// Für Ultrasonic Sensor
#include "SR04.h"   

// Für Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

SR04 sr04 = SR04(ECHO, TRIG);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WebServer server(80);

// --- Roboter-Zustand ---
bool roboterAktiv  = false;   // true = fährt autonom
bool notStopp      = false;   // true = gesperrt bis Reset

// --- Motor-Trimm ---
// Positiv = rechts zu stark → linker Motor bekommt Boost
// Negativ = links zu stark  → rechter Motor bekommt Boost
int trimmWert = 0;   // Bereich: -20 bis +20

// ============================================================
//  Display-Anzeige 
// ============================================================
 
String letzterDisplayText = "";   // Merkt sich den zuletzt angezeigten Text
 
void displayShow(String text) {
  if (text == letzterDisplayText) return;   // Nichts tun wenn Text gleich
  letzterDisplayText = text;
  Serial.println(text);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(text.c_str());
  display.display();
}

// ============================================================
//  Motorsteuerung – mit PWM
// ============================================================

void motorenStopp() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(PWM_KANAL_A, 0);
  ledcWrite(PWM_KANAL_B, 0);
}

void vorwaerts() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
// Trimm: positiver Wert → links schneller, rechts langsamer (Korrektur Rechtskurve)
  int pwmLinks  = constrain(FAHRGESCHWINDIGKEIT + trimmWert, 0, 255);
  int pwmRechts = constrain(FAHRGESCHWINDIGKEIT - trimmWert, 0, 255);
  ledcWrite(PWM_KANAL_A, pwmLinks);
  ledcWrite(PWM_KANAL_B, pwmRechts);
}

void rueckwaerts() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  int pwmLinks  = constrain(FAHRGESCHWINDIGKEIT + trimmWert, 0, 255);
  int pwmRechts = constrain(FAHRGESCHWINDIGKEIT - trimmWert, 0, 255);
  ledcWrite(PWM_KANAL_A, pwmLinks);
  ledcWrite(PWM_KANAL_B, pwmRechts);
}

// Rechtsdrehung: Motor A vorwärts, Motor B rückwärts
void dreheRechts() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  ledcWrite(PWM_KANAL_A, DREHGESCHWINDIGKEIT);
  ledcWrite(PWM_KANAL_B, DREHGESCHWINDIGKEIT);
}

// Linksdrehung: Motor A rückwärts, Motor B vorwärts
void dreheLinks() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(PWM_KANAL_A, DREHGESCHWINDIGKEIT);
  ledcWrite(PWM_KANAL_B, DREHGESCHWINDIGKEIT);
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
//  Webserver-Routen
// ============================================================

void handleRoot() {
  server.send(200, "text/html", HTML_SEITE);
}

void handleStart() {
  if (notStopp) {
    server.send(200, "text/plain", "⛔ Not-Stopp aktiv – zuerst Reset!");
    return;
  }
  roboterAktiv = true;
  Serial.println("[WEB] Start");
  server.send(200, "text/plain", "▶ Roboter fährt");
}

void handleStopp() {
  roboterAktiv = false;
  displayShow("Stopp");
  motorenStopp();
  Serial.println("[WEB] Stopp");
  server.send(200, "text/plain", "⏹ Gestoppt");
}

void handleNotStopp() {
  roboterAktiv = false;
  notStopp     = true;
  displayShow("NOT-STOPP");
  motorenStopp();
  Serial.println("[WEB] *** NOT-STOPP ***");
  server.send(200, "text/plain", "⛔ NOT-STOPP! Reset zum Freigeben.");
}

void handleReset() {
  notStopp = false;
  Serial.println("[WEB] Reset – bereit");
  server.send(200, "text/plain", "↺ Bereit – Start drücken");
}

void handleAbstand() {
  int a = sr04.Distance();
  server.send(200, "text/plain", String(a));
}

void handleTrimm() {
  if (server.hasArg("wert")) {
    int wert = server.arg("wert").toInt();
    trimmWert = constrain(wert, -20, 20);
    Serial.print("[WEB] Trimm: "); Serial.println(trimmWert);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Kein Wert");
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

  // Richtungspins
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // PWM-Kanäle einrichten (ESP32 LEDC)
  ledcSetup(PWM_KANAL_A, PWM_FREQ, PWM_AUFLOES);
  ledcSetup(PWM_KANAL_B, PWM_FREQ, PWM_AUFLOES);
  ledcAttachPin(ENA, PWM_KANAL_A);
  ledcAttachPin(ENB, PWM_KANAL_B);

  motorenStopp();
  delay(2000);

  // WLAN Access Point starten
  WiFi.softAP(WLAN_SSID, WLAN_PASSWORT, WLAN_KANAL, WLAN_SSID_HIDDEN, WLAN_MAX_CLIENTS);
  displayShow("WLAN gestartet\nIP: " + WiFi.softAPIP().toString());   // Immer 192.168.4.1

  // Routen registrieren
  server.on("/",         handleRoot);
  server.on("/start",    handleStart);
  server.on("/stopp",    handleStopp);
  server.on("/notstopp", handleNotStopp);
  server.on("/reset",    handleReset);
  server.on("/abstand",  handleAbstand);
  server.on("/trimm",    handleTrimm);
  server.begin();

}

// ============================================================
//  Hauptschleife
// ============================================================

void loop() {
  server.handleClient();   // WLAN-Anfragen immer zuerst bedienen!
  
  if (!roboterAktiv || notStopp) return;
  
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

    // Webserver kurz bedienen, damit Not-Stopp sofort reagiert
    server.handleClient();
    if (!roboterAktiv || notStopp) return;

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
