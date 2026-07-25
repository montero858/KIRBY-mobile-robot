// ============================================================
//  IrSensor.h
//  Abstraktion für TCRT5000-basierte IR-Reflexionssensoren
//
//  Unterstützt zwei semantisch gegensätzliche Betriebsmodi:
//
//  Mode::OBSTACLE  – Alarm wenn Reflexion erkannt wird
//                    (Objekt in der Nähe)
//                    Typische Verwendung: seitliche Hindernisse
//
//  Mode::FLOOR     – Alarm wenn KEINE Reflexion erkannt wird
//                    (kein Boden vorhanden = Absturzgefahr!)
//                    Typische Verwendung: Tischkanten-Schutz
//
//  Entstörung: isTriggered() wertet zwei aufeinanderfolgende
//  Lesungen aus. Beide müssen übereinstimmen, damit ein Alarm
//  als gültig gilt. Dies verhindert Fehlauslösungen durch
//  kurze Störimpulse.
// ============================================================

#pragma once

#include <Arduino.h>

class IrSensor {
public:
    // Betriebsmodus
    enum class Mode {
        OBSTACLE,   // Alarm wenn Reflexion da (Objekt nah)
        FLOOR       // Alarm wenn keine Reflexion (kein Boden)
    };

    // --------------------------------------------------------
    //  Konstruktor
    //  pin            : GPIO-Pin (Input)
    //  mode           : OBSTACLE oder FLOOR (s.o.)
    //  reflectionLevel: elektrischer Pegel bei Reflexion
    //                   (TCRT5000 typisch: LOW)
    //  debounceUs     : Pause zwischen den zwei Entstörungs-
    //                   Lesungen in Mikrosekunden (default 500)
    // --------------------------------------------------------
    IrSensor(uint8_t pin, Mode mode,
             uint8_t reflectionLevel = LOW,
             uint16_t debounceUs = 500);

    // Pin als Input initialisieren
    void begin();

    // --------------------------------------------------------
    //  isTriggered()  – Entstörte Abfrage
    //  Gibt true zurück wenn der Sensor einen Alarm-Zustand
    //  meldet (Bedeutung je nach Mode, s.o.).
    //  Zwei aufeinanderfolgende Lesungen müssen übereinstimmen.
    // --------------------------------------------------------
    bool isTriggered();

    // --------------------------------------------------------
    //  rawReflection()  – Ungefilterte Einzellesung
    //  Gibt true zurück wenn eine Reflexion erkannt wird
    //  (Boden/Objekt vorhanden), unabhängig vom Mode.
    //  Geeignet für Statusanzeigen, nicht für sicherheits-
    //  relevante Entscheidungen.
    // --------------------------------------------------------
    bool rawReflection();

private:
    uint8_t  _pin;
    Mode     _mode;
    uint8_t  _reflectionLevel;
    uint16_t _debounceUs;
};
