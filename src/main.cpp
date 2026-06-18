// ============================================================
//  KIRBY - Selbstfahrender Roboter
//  Hardware: ESP32 + L298N + 1x HC-SR04 (auf SG90-Servo)
//            + 7x IR-Sensoren + SSD1306 Display
//  Steuerung: Browser unter http://192.168.4.1
//
//  Sensor-Layout:
//    IR vorne mitte/links/rechts   : Hindernis (binaer)
//    IR vorne links/rechts         : Boden (Absturz)
//    IR hinten links/rechts        : Boden (Absturz)
//    HC-SR04 auf Servo, vorne mitte: Abstand (schwenkbar)
//
//  Prioritaeten (hoch -> niedrig):
//    1. Boden vorne UND hinten gleichzeitig -> Notstopp
//    2. Boden vorne   -> zurueck, 90 Grad drehen, weiterfahren
//    3. Boden hinten  -> langsam vorwaerts
//    4. IR-Hindernis schraeg vorne -> Stopp + Servo-Scan + Drehen
//    5. Ultraschall STOP-Schwelle  -> Stopp + Servo-Scan + Drehen
//    6. Ultraschall SLOW-Schwelle  -> Geschwindigkeit reduzieren
//    7. Frei -> Geradeausfahrt
// ============================================================

#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include "WiFiControl.h"
#include "SR04.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

SR04 sensor = SR04(ECHO, TRIG);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// --- Roboter-Zustand ---
bool roboterAktiv = false;

// --- Geschwindigkeit ---
int maxGeschwindigkeit = FAHRGESCHWINDIGKEIT;   // per Stufen-Button
int geschwindigkeit    = FAHRGESCHWINDIGKEIT;   // aktuell berechnet

// --- Per Web abstimmbare Parameter ---
int schwelleFrontSlow   = SCHWELLE_FRONT_SLOW_DEFAULT;
int schwelleFrontStop   = SCHWELLE_FRONT_STOP_DEFAULT;
int drehgeschwindigkeit = DREHGESCHWINDIGKEIT_DEFAULT;

// --- Letzter Motor-Zustand (verhindert redundante Schreibzugriffe) ---
int  letzterPwmA    = -1;
int  letzterPwmB    = -1;
bool letzterRueckwA = false;
bool letzterRueckwB = false;

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
//  Servo-Steuerung
//  Manuell per LEDC (kein zusaetzliches Servo.h noetig) -
//  konsistent mit der bestehenden PWM-Ansteuerung der Motoren.
// ============================================================

void servoSchreibeWinkel(int winkel) {
  winkel = constrain(winkel, 0, 180);
  // Standard-Servo: 1000us (0 Grad) bis 2000us (180 Grad) bei 20ms Periode
  int pulsbreiteUs = 1000 + (winkel * 1000) / 180;
  int duty = (pulsbreiteUs * 65536) / 20000;   // 16-Bit Aufloesung, 20ms Periode
  ledcWrite(PWM_KANAL_SERVO, duty);
}

// ============================================================
//  Motorsteuerung (L298N)
// ============================================================

void setzeMotoren(int pwmA, int pwmB, bool ruckwA = false, bool ruckwB = false) {
  // Nur schreiben wenn sich der Zustand wirklich geaendert hat
  if (pwmA == letzterPwmA && pwmB == letzterPwmB &&
      ruckwA == letzterRueckwA && ruckwB == letzterRueckwB) return;

  letzterPwmA    = pwmA;
  letzterPwmB    = pwmB;
  letzterRueckwA = ruckwA;
  letzterRueckwB = ruckwB;

  if      (pwmA == 0) { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  }
  else if (ruckwA)    { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  else                 { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }

  if      (pwmB == 0) { digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);  }
  else if (ruckwB)    { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  else                 { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }

  ledcWrite(PWM_KANAL_A, pwmA);
  ledcWrite(PWM_KANAL_B, pwmB);
}

void motorenStopp()     { setzeMotoren(0, 0); }
void vorwaerts(int v)   { setzeMotoren(v, v, false, false); }
void rueckwaerts(int v) { setzeMotoren(v, v, true,  true);  }
void dreheRechts(int v) { setzeMotoren(v, v, false, true);  }
void dreheLinks(int v)  { setzeMotoren(v, v, true,  false); }

// ============================================================
//  Ultraschallmessung - Mittelwert mit Fehlerbehandlung
// ============================================================

int messeAbstand() {
  int a = sensor.Distance();
  delay(SENSOR_DELAY_MS);
  int b = sensor.Distance();
  bool aUng = (a <= 0 || a > 400);
  bool bUng = (b <= 0 || b > 400);
  if (aUng && bUng) return 999;
  if (aUng) return b;
  if (bUng) return a;
  return (a + b) / 2;
}

// ============================================================
//  IR-Sensoren auslesen
// ============================================================

// Entstoerte Messung: zwei Lesungen muessen uebereinstimmen,
// bevor ein sicherheitsrelevanter Trigger ausgeloest wird
bool irErkannt(int pin) {
  bool erste = (digitalRead(pin) == IR_PEGEL_ERKANNT);
  delayMicroseconds(500);
  bool zweite = (digitalRead(pin) == IR_PEGEL_ERKANNT);
  return erste && zweite;
}

// ============================================================
//  Adaptive Geschwindigkeit
//  Linear von maxGeschwindigkeit -> MIN_GESCHWINDIGKEIT
//  zwischen SLOW- und STOP-Schwelle
// ============================================================

/*
int berechneGeschwindigkeit(int abstand) {
  if (abstand >= schwelleFrontSlow) return maxGeschwindigkeit;
  if (abstand <= schwelleFrontStop) return MIN_GESCHWINDIGKEIT;
  int bereich = schwelleFrontSlow - schwelleFrontStop;
  int anteil  = abstand - schwelleFrontStop;
  return MIN_GESCHWINDIGKEIT +
         (maxGeschwindigkeit - MIN_GESCHWINDIGKEIT) * anteil / bereich;
}
*/

// ============================================================
//  Servo-Scan: Kopf nach rechts/links schwenken und messen,
//  zurueck zur Mitte, dann Roboter in freiere Richtung drehen.
//  Der Servo uebernimmt das Messen, der Roboter dreht erst
//  danach physisch in die ermittelte Richtung.
// ============================================================
void scanneUndDrehe() {
  int delayfactor = 1;
  displayShow("Scanne rechts...");
  servoSchreibeWinkel(SERVO_WINKEL_RECHTS);
  delay(SERVO_BEWEGUNGSZEIT_MS);
  int abstandRechts = messeAbstand();

  displayShow("Scanne links...");
  servoSchreibeWinkel(SERVO_WINKEL_LINKS);
  delay(SERVO_BEWEGUNGSZEIT_MS * 2);   // laengerer Weg von rechts nach links
  int abstandLinks = messeAbstand();

  Serial.print("  Scan R:"); Serial.print(abstandRechts);
  Serial.print(" L:"); Serial.println(abstandLinks);

  if (abstandLinks < schwelleFrontStop && abstandRechts < schwelleFrontStop) {
    displayShow("Umkehren!");
    rueckwaerts(LANGSAM_GESCHWINDIGKEIT);
    delay(600);
    motorenStopp();
    delay(200);
    dreheRechts(drehgeschwindigkeit);
    delayfactor = 2;  // laenger drehen, da beide Seiten blockiert
  } 
  else if (abstandLinks >= abstandRechts) {
    displayShow("Drehe links");
    dreheLinks(drehgeschwindigkeit);
  } 
  else {
    displayShow("Drehe rechts");
    dreheRechts(drehgeschwindigkeit);
  }

  servoSchreibeWinkel(SERVO_WINKEL_MITTE);

  if (SERVO_BEWEGUNGSZEIT_MS > DREH_DAUER_MS) {
    delay(SERVO_BEWEGUNGSZEIT_MS * delayfactor);
  } else {
    delay(DREH_DAUER_MS * delayfactor);
  }  
  motorenStopp();
  delay(200);
}

void einfacherScanUndDrehe(String richtung) {
  int delayfactor = 1;
  displayShow("Scanne " + richtung);
  if (richtung == "rechts") {
    servoSchreibeWinkel(SERVO_WINKEL_RECHTS);
  } else {  
    servoSchreibeWinkel(SERVO_WINKEL_LINKS);
  }
  
  delay(SERVO_BEWEGUNGSZEIT_MS);
  int abstand = messeAbstand();

  Serial.print("  Scan :"); Serial.print(abstand);
 
  if (abstand > schwelleFrontStop) {
    displayShow("Drehe " + richtung);
    if (richtung == "rechts") {
      dreheRechts(drehgeschwindigkeit);
    } else {
      dreheLinks(drehgeschwindigkeit);
    }
 
  }
  else{
    // Kein freier Weg --> Umkehren
    dreheRechts(drehgeschwindigkeit);
    delayfactor = 2;  // laenger drehen, da beide Seiten blockiert
  }

  servoSchreibeWinkel(SERVO_WINKEL_MITTE);

  if (SERVO_BEWEGUNGSZEIT_MS > DREH_DAUER_MS) {
    delay(SERVO_BEWEGUNGSZEIT_MS * delayfactor);
  } else {
    delay(DREH_DAUER_MS * delayfactor);
  }   
  motorenStopp();
  delay(200);
}

// ============================================================
//  Absturz-Routinen (Boden-IR-Sensoren)
// ============================================================

void routineBoden(String richtung, String sensorSeite) {
  displayShow("Abgrund " + richtung + "!\nWeiche aus...");
  motorenStopp();
  delay(200);
  if (richtung == "vorne") {
    rueckwaerts(LANGSAM_GESCHWINDIGKEIT);
  }
  else {
    // richtung == "hinten"
    vorwaerts(LANGSAM_GESCHWINDIGKEIT);
  }
  delay(LANGSAME_FAHRT_DAUER_MS);
  motorenStopp();
  delay(200);

  if (sensorSeite == "mitte") {
    // Kein Seitenunterschied -> Richtung egal, einfach drehen
    displayShow("Drehe...");
    dreheRechts(drehgeschwindigkeit);
    delay(DREH_DAUER_MS * 2);
  }
  else if (sensorSeite == "rechts") {
    // Mehr Platz links -> nach links drehen
    displayShow("Drehe links...");
    dreheLinks(drehgeschwindigkeit);
    delay(DREH_DAUER_MS);
  }
  else {
    // Mehr Platz rechts -> nach rechts drehen
    displayShow("Drehe rechts...");
    dreheRechts(drehgeschwindigkeit);
    delay(DREH_DAUER_MS);
  }
  motorenStopp();
  delay(200);
  return;
}

// ============================================================
//  Webserver-Routen
// ============================================================

void handleRoot() {
  server.send(200, "text/html", HTML_SEITE);
}

void handleStart() {
  roboterAktiv = true;
  servoSchreibeWinkel(SERVO_WINKEL_MITTE);
  displayShow("Start");
  server.send(200, "text/plain", "Roboter faehrt");
}

void handleStopp() {
  roboterAktiv = false;
  motorenStopp();
  displayShow("Stopp");
  server.send(200, "text/plain", "Gestoppt");
}

void handleScan() {
  server.send(200, "text/plain", "Scan gestartet...");
  motorenStopp();
  delay(200);
  scanneUndDrehe();
}

void handleAbstand() {
  int u = messeAbstand();
  String a = "U:"  + String(u) +
            " HL:" + String(irErkannt(IR_HINDERNIS_LINKS)) +
            " HM:" + String(irErkannt(IR_HINDERNIS_MITTE)) +  // ← fehlt
            " HR:" + String(irErkannt(IR_HINDERNIS_RECHTS));
  server.send(200, "text/plain", a);
}

void handleStufe() {
  if (!server.hasArg("wert")) { server.send(400, "text/plain", "Kein Wert"); return; }
  switch (server.arg("wert").toInt()) {
    case 1: maxGeschwindigkeit = 120; break;
    case 2: maxGeschwindigkeit = 165; break;
    case 3: maxGeschwindigkeit = 210; break;
    case 4: maxGeschwindigkeit = 255; break;
    default: server.send(400, "text/plain", "Ungueltige Stufe"); return;
  }
  server.send(200, "text/plain", "OK");
}

void handleParameter() {
  if (server.hasArg("fs")) schwelleFrontSlow   = constrain(server.arg("fs").toInt(), 1, 400);
  if (server.hasArg("fp")) schwelleFrontStop   = constrain(server.arg("fp").toInt(), 1, 400);
  if (server.hasArg("dg")) drehgeschwindigkeit = constrain(server.arg("dg").toInt(), 0, 255);

  String json = "{";
  json += "\"fs\":" + String(schwelleFrontSlow)   + ",";
  json += "\"fp\":" + String(schwelleFrontStop)   + ",";
  json += "\"dg\":" + String(drehgeschwindigkeit);
  json += "}";
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

  // Motorpins
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcSetup(PWM_KANAL_A, PWM_FREQ, PWM_AUFLOES);
  ledcSetup(PWM_KANAL_B, PWM_FREQ, PWM_AUFLOES);
  ledcAttachPin(ENA, PWM_KANAL_A);
  ledcAttachPin(ENB, PWM_KANAL_B);

  // Servo
  ledcSetup(PWM_KANAL_SERVO, SERVO_FREQ, SERVO_AUFLOES);
  ledcAttachPin(SERVO_PIN, PWM_KANAL_SERVO);
  servoSchreibeWinkel(SERVO_WINKEL_MITTE);

  // Ultraschall
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // IR-Sensoren
  pinMode(IR_HINDERNIS_LINKS,     INPUT);
  pinMode(IR_HINDERNIS_RECHTS,    INPUT);
  pinMode(IR_HINDERNIS_MITTE,     INPUT);
  pinMode(IR_BODEN_VORNE_LINKS,   INPUT);
  pinMode(IR_BODEN_VORNE_RECHTS,  INPUT);
  pinMode(IR_BODEN_HINTEN_LINKS,  INPUT);
  pinMode(IR_BODEN_HINTEN_RECHTS, INPUT);

  motorenStopp();
  delay(1500);

  WiFi.softAP(WLAN_SSID, WLAN_PASSWORT, WLAN_KANAL, WLAN_SSID_HIDDEN, WLAN_MAX_CLIENTS);
  displayShow("WLAN\nIP: " + WiFi.softAPIP().toString());

  server.on("/",          handleRoot);
  server.on("/start",     handleStart);
  server.on("/stopp",     handleStopp);
  server.on("/scan",      handleScan);
  server.on("/abstand",   handleAbstand);
  server.on("/stufe",     handleStufe);
  server.on("/parameter", handleParameter);
  server.begin();
}

// ============================================================
//  Hauptschleife
// ============================================================

void loop() {
  server.handleClient();
  if (!roboterAktiv) return;

  // Einmalig alle Sensoren lesen
  bool bvL = irErkannt(IR_BODEN_VORNE_LINKS);
  bool bvR = irErkannt(IR_BODEN_VORNE_RECHTS);
  bool bhL = irErkannt(IR_BODEN_HINTEN_LINKS);
  bool bhR = irErkannt(IR_BODEN_HINTEN_RECHTS);
  bool hiL = irErkannt(IR_HINDERNIS_LINKS);
  bool hiR = irErkannt(IR_HINDERNIS_RECHTS);
  bool hiM = irErkannt(IR_HINDERNIS_MITTE);

  // Prioritaet 1: Boden vorne UND hinten gleichzeitig -> Notstopp
  bool bv = bvL   || bvR;
  bool bh = bhL  || bhR;
  if (bv && bh) {
      motorenStopp();
      displayShow("NOTSTOPP\nBoden vorn+hinten");
      return;   
  }

  // Prioritaet 2: Boden vorne -> ausweichen
  if (bvL && bvR) {
    routineBoden("vorne", "mitte");
    return;
  }
  else if (bvR && !bvL) {
    routineBoden("vorne", "rechts");
    return;
  }
  else if (bvL && !bvR) {
    routineBoden("vorne", "links");
    return;
  }
  else{
    // Kein Boden vorne erkannt, weiter mit den anderen Prioritaeten
  }

  // Prioritaet 3: Boden hinten -> vorfahren
  if (bhL && bhR) {
    routineBoden("hinten", "mitte");
    return;
  }
  else if (bhR && !bhL) {
    routineBoden("hinten", "rechts");
    return;
  }
  else if (bhL && !bhR) {
    routineBoden("hinten", "links");
    return;
  }
  else{
    // Kein Boden hinten erkannt, weiter mit den anderen Prioritaeten
  }

  // Prioritaet 4: IR-Hindernis schraeg vorne -> Stopp + Scan
  if (hiR) {
    motorenStopp();
    displayShow("Hindernis IR!\nScanne...");
    delay(200);
    einfacherScanUndDrehe("links");
    return;
  }

  if (hiL) {
    motorenStopp();
    displayShow("Hindernis IR!\nScanne...");
    delay(200);
    einfacherScanUndDrehe("rechts");
    return;
  }

  // Prioritaet 5: Ultraschall vorne
  int abstand = messeAbstand();

  if ((abstand < schwelleFrontStop) || hiM) {
    motorenStopp();
    displayShow("BLOCKIERT!");
    delay(200);
    scanneUndDrehe();
    return;
  }

  /*
  geschwindigkeit = berechneGeschwindigkeit(abstand);
  displayShow("Fahre v:" + String(geschwindigkeit) +
              "\nU:" + String(abstand) + "cm");
  vorwaerts(geschwindigkeit);
  */
  vorwaerts(maxGeschwindigkeit);
}