// ============================================================
//  KIRBY – Selbstfahrender Roboter
//  Hardware: ESP32 + L298N + 3x HC-SR04 + Display
//  Steuerung: Browser unter http://192.168.4.1
//
//  Sensoranordnung:
//    [SR04-L]  [SR04-M]  [SR04-R]
//    leicht    gerade    leicht
//    links           rechts gedreht
//
//  Fahrlogik (Prioritaet hoch -> niedrig):
//    1. Irgendein Sensor < STOP-Schwelle  -> Stopp + Scan
//    2. Seitensensor < LIMIT-Schwelle     -> Gegenlenken
//    3. Frontsensor  < LIMIT-Schwelle     -> Kurve zur freieren Seite
//    4. Alles frei                        -> Geradeaus
// ============================================================

#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include "WiFiControl.h"
#include "SR04.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Drei Ultraschallsensoren ---
SR04 sensorM = SR04(ECHO_M, TRIG_M);
SR04 sensorR = SR04(ECHO_R, TRIG_R);
SR04 sensorL = SR04(ECHO_L, TRIG_L);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// --- Roboter-Zustand ---
bool roboterAktiv = false;

// --- Geschwindigkeit (per Web einstellbar) ---
int geschwindigkeit      = FAHRGESCHWINDIGKEIT;

// --- Abstimmbare Parameter (per Web einstellbar) ---
int schwelleFrontLimit   = SCHWELLE_FRONT_LIMIT_DEFAULT;
int schwelleFrontStop    = SCHWELLE_FRONT_STOP_DEFAULT;
int schwelleSeiteLimit   = SCHWELLE_SEITE_LIMIT_DEFAULT;
int schwelleSeiteStop    = SCHWELLE_SEITE_STOP_DEFAULT;
int drehgeschwindigkeit  = DREHGESCHWINDIGKEIT_DEFAULT;
int kurveInnenProzent    = KURVE_INNEN_PROZENT_DEFAULT;

// --- Gecachte Sensorwerte (fuer handleAbstand ohne neue Messung) ---
int cachedAbstandM = 999;
int cachedAbstandR = 999;
int cachedAbstandL = 999;

// --- Letzter Fahrzustand (verhindert redundante Motor-Writes) ---
// Moegliche Werte: 'F'=vorwaerts, 'L'=kurveLinks, 'R'=kurveRechts,
//                 'S'=stopp, 'U'=rueckwaerts, 'D'=drehen (scan)
char letzterFahrzustand = 'S';

// ============================================================
//  Display
// ============================================================

String letzterDisplayText = "";

void displayShow(String text) {
  if (text == letzterDisplayText) return;
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
//  Motorsteuerung – mit Zustandsmerker
// ============================================================

// Gemeinsame PWM-Hilfsfunktion
void setzePWM(int pwmA, int pwmB) {
  ledcWrite(PWM_KANAL_A, pwmA);
  ledcWrite(PWM_KANAL_B, pwmB);
}

void motorenStopp() {
  if (letzterFahrzustand == 'S') return;
  letzterFahrzustand = 'S';
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  setzePWM(0, 0);
}

void vorwaerts() {
  if (letzterFahrzustand == 'F') return;
  letzterFahrzustand = 'F';
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setzePWM(geschwindigkeit, geschwindigkeit);
}

void rueckwaerts() {
  if (letzterFahrzustand == 'U') return;
  letzterFahrzustand = 'U';
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setzePWM(geschwindigkeit, geschwindigkeit);
}

void dreheRechts() {
  letzterFahrzustand = 'D';
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  setzePWM(drehgeschwindigkeit, drehgeschwindigkeit);
}

void dreheLinks() {
  letzterFahrzustand = 'D';
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setzePWM(drehgeschwindigkeit, drehgeschwindigkeit);
}

// Gemeinsame Kurven-Hilfsfunktion
void setzeKurve(char seite) {
  if (letzterFahrzustand == seite) return;
  letzterFahrzustand = seite;
  int pwmInnen = (geschwindigkeit * kurveInnenProzent) / 100;
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  if (seite == 'R') {
    setzePWM(geschwindigkeit, pwmInnen);  // links aussen, rechts innen
  } else {
    setzePWM(pwmInnen, geschwindigkeit);  // links innen, rechts aussen
  }
}

void kurveRechts() { setzeKurve('R'); }
void kurveLinks()  { setzeKurve('L'); }

// ============================================================
//  Ultraschallmessung – Mittelwert, mit Fehlerbehandlung
// ============================================================

int messeAbstand(SR04 &sensor) {
  int a = sensor.Distance();
  delay(15);
  int b = sensor.Distance();

  // Einzelne ungueltige Werte durch den anderen ersetzen
  bool aUngueltig = (a <= 0 || a > 400);
  bool bUngueltig = (b <= 0 || b > 400);

  if (aUngueltig && bUngueltig) return 999;  // beide ungueltig -> kein Hindernis
  if (aUngueltig) return b;
  if (bUngueltig) return a;
  return (a + b) / 2;
}

// Alle drei Sensoren sequenziell auslesen und Cache aktualisieren
void aktualisiereAbstaende() {
  cachedAbstandM = messeAbstand(sensorM);
  delay(SENSOR_DELAY_MS);
  cachedAbstandR = messeAbstand(sensorR);
  delay(SENSOR_DELAY_MS);
  cachedAbstandL = messeAbstand(sensorL);
  delay(SENSOR_DELAY_MS);
}

// ============================================================
//  Scan: Physisch drehen und freie Richtung suchen
//  Wird nur bei STOP-Schwelle aufgerufen
// ============================================================

void scanneUndDrehe() {
  // --- Scan rechts ---
  displayShow("Scanne rechts...");
  dreheRechts();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);
  int abstandRechts = messeAbstand(sensorM);

  // Not-Stopp-Pruefung mitten im Scan
  if (!roboterAktiv) return;

  // Zurueck zur Mitte
  dreheLinks();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);

  // --- Scan links ---
  displayShow("Scanne links...");
  dreheLinks();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);
  int abstandLinks = messeAbstand(sensorM);

  // Not-Stopp-Pruefung mitten im Scan
  if (!roboterAktiv) return;

  // Zurueck zur Mitte
  dreheRechts();
  delay(DREH_DAUER_MS);
  motorenStopp();
  delay(SCAN_PAUSE_MS);

  Serial.print("  Scan R:"); Serial.print(abstandRechts);
  Serial.print(" L:"); Serial.println(abstandLinks);

  // Entscheidung
  if (abstandLinks < schwelleFrontStop && abstandRechts < schwelleFrontStop) {
    displayShow("Umkehren!");
    rueckwaerts();
    delay(600);
    motorenStopp();
    delay(200);
    dreheRechts();
    delay(DREH_DAUER_MS * 2);
  } else if (abstandLinks >= abstandRechts) {
    displayShow("Drehe links");
    dreheLinks();
    delay(DREH_DAUER_MS);
  } else {
    displayShow("Drehe rechts");
    dreheRechts();
    delay(DREH_DAUER_MS);
  }

  motorenStopp();
  delay(300);
}

// ============================================================
//  Webserver-Routen
// ============================================================

void handleRoot() {
  server.send(200, "text/html", HTML_SEITE);
}

void handleStart() {
  roboterAktiv = true;
  displayShow("Start");
  Serial.println("[WEB] Start");
  server.send(200, "text/plain", "Roboter faehrt");
}

void handleStopp() {
  roboterAktiv = false;
  displayShow("Stopp");
  motorenStopp();
  Serial.println("[WEB] Stopp");
  server.send(200, "text/plain", "Gestoppt");
}

void handleAbstand() {
  // Gecachte Werte aus letztem Loop-Durchlauf senden – keine neue Messung
  String antwort = "M:" + String(cachedAbstandM) +
                   " R:" + String(cachedAbstandR) +
                   " L:" + String(cachedAbstandL);
  server.send(200, "text/plain", antwort);
}

void handleGeschwindigkeit() {
  if (server.hasArg("wert")) {
    int wert = server.arg("wert").toInt();
    geschwindigkeit = constrain(wert, 0, 255);
    letzterFahrzustand = 'S';
    Serial.print("[WEB] Speed: "); Serial.println(geschwindigkeit);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Kein Wert");
  }
}

void handleParameter() {
  // Jeden Parameter nur setzen wenn er in der Anfrage enthalten ist
  if (server.hasArg("fl"))  schwelleFrontLimit  = constrain(server.arg("fl").toInt(),  1, 400);
  if (server.hasArg("fs"))  schwelleFrontStop   = constrain(server.arg("fs").toInt(),  1, 400);
  if (server.hasArg("sl"))  schwelleSeiteLimit  = constrain(server.arg("sl").toInt(),  1, 400);
  if (server.hasArg("ss"))  schwelleSeiteStop   = constrain(server.arg("ss").toInt(),  1, 400);
  if (server.hasArg("dg"))  drehgeschwindigkeit = constrain(server.arg("dg").toInt(),  0, 255);
  if (server.hasArg("kp"))  kurveInnenProzent   = constrain(server.arg("kp").toInt(),  0, 100);

  // Aktuelle Werte als JSON zuruecksenden damit Web-Interface Bestaetigung hat
  String json = "{";
  json += "\"fl\":"  + String(schwelleFrontLimit)  + ",";
  json += "\"fs\":"  + String(schwelleFrontStop)   + ",";
  json += "\"sl\":"  + String(schwelleSeiteLimit)  + ",";
  json += "\"ss\":"  + String(schwelleSeiteStop)   + ",";
  json += "\"dg\":"  + String(drehgeschwindigkeit) + ",";
  json += "\"kp\":"  + String(kurveInnenProzent);
  json += "}";

  Serial.println("[WEB] Parameter: " + json);
  server.send(200, "application/json", json);
}

// ============================================================
//  Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  displayShow("Initialisierung");

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  ledcSetup(PWM_KANAL_A, PWM_FREQ, PWM_AUFLOES);
  ledcSetup(PWM_KANAL_B, PWM_FREQ, PWM_AUFLOES);
  ledcAttachPin(ENA, PWM_KANAL_A);
  ledcAttachPin(ENB, PWM_KANAL_B);

  motorenStopp();
  delay(2000);

  WiFi.softAP(WLAN_SSID, WLAN_PASSWORT, WLAN_KANAL, WLAN_SSID_HIDDEN, WLAN_MAX_CLIENTS);
  displayShow("WLAN gestartet\nIP: " + WiFi.softAPIP().toString());

  server.on("/",                handleRoot);
  server.on("/start",           handleStart);
  server.on("/stopp",           handleStopp);
  server.on("/abstand",         handleAbstand);
  server.on("/geschwindigkeit", handleGeschwindigkeit);
  server.on("/parameter",       handleParameter);
  server.begin();
}

// ============================================================
//  Hauptschleife
// ============================================================

void loop() {
  server.handleClient();
  if (!roboterAktiv) return;

  // Alle drei Sensoren sequenziell auslesen und cachen
  aktualisiereAbstaende();

  Serial.print("M:"); Serial.print(cachedAbstandM);
  Serial.print(" R:"); Serial.print(cachedAbstandR);
  Serial.print(" L:"); Serial.println(cachedAbstandL);

  // ── Prioritaet 1: STOP-Schwelle – sofort anhalten und scannen ──
  if (cachedAbstandM < schwelleFrontStop ||
      cachedAbstandR < schwelleSeiteStop ||
      cachedAbstandL < schwelleSeiteStop) {

    displayShow("BLOCKIERT!\nStoppe...");
    motorenStopp();
    delay(300);

    server.handleClient();
    if (!roboterAktiv) return;

    scanneUndDrehe();
    return;
  }

  // ── Prioritaet 2: LIMIT-Schwelle Seite – Gegenlenken ──
  if (cachedAbstandL < schwelleSeiteLimit && cachedAbstandR >= schwelleSeiteLimit) {
    displayShow("Wand links\nlenke rechts");
    kurveRechts();
    return;
  }

  if (cachedAbstandR < schwelleSeiteLimit && cachedAbstandL >= schwelleSeiteLimit) {
    displayShow("Wand rechts\nlenke links");
    kurveLinks();
    return;
  }

  // ── Prioritaet 3: LIMIT-Schwelle Front – Kurve zur freieren Seite ──
  if (cachedAbstandM < schwelleFrontLimit) {
    if (cachedAbstandL >= cachedAbstandR) {
      displayShow("Front nah\nKurve links");
      kurveLinks();
    } else {
      displayShow("Front nah\nKurve rechts");
      kurveRechts();
    }
    return;
  }

  // ── Prioritaet 4: Freie Fahrt ──
  displayShow("Fahre geradeaus");
  vorwaerts();
}