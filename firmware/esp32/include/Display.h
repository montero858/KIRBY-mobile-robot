// ============================================================
//  Display.h
//  Wrapper um den SSD1306 OLED-Treiber.
//  Schreibt nur dann neu auf das Display, wenn sich der
//  angezeigte Text tatsaechlich geaendert hat.
// ============================================================

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
public:
    // --------------------------------------------------------
    //  Konstruktor
    //  sda/scl   : I2C-Pins
    //  i2cAddr   : I2C-Adresse (typisch 0x3C)
    //  width/height: Aufloesung in Pixeln
    // --------------------------------------------------------
    Display(uint8_t sda, uint8_t scl,
            uint8_t i2cAddr  = 0x3C,
            uint8_t width    = 128,
            uint8_t height   = 64);

    // Hardware und I2C initialisieren.
    // Gibt false zurueck wenn das Display nicht antwortet.
    bool begin();

    // Text anzeigen (mehrzeilig mit '\n' moeglich).
    // Kein Neuzeichnen wenn Text unveraendert.
    void show(const String& text);

    // Display leeren (ohne Zustandspruefung).
    void clear();

private:
    uint8_t          _sda;
    uint8_t          _scl;
    uint8_t          _addr;
    Adafruit_SSD1306 _oled;
    String           _lastText;
};
