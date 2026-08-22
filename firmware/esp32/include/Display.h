// ============================================================
//  Display.h
//  Wrapper around the SSD1306 OLED driver.
//  Only redraws the display when the displayed text has
//  actually changed.
// ============================================================

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
public:
    // --------------------------------------------------------
    //  Constructor
    //  sda/scl   : I2C pins
    //  i2cAddr   : I2C address (typically 0x3C)
    //  width/height: resolution in pixels
    // --------------------------------------------------------
    Display(uint8_t sda, uint8_t scl,
            uint8_t i2cAddr  = 0x3C,
            uint8_t width    = 128,
            uint8_t height   = 64);

    // Initialize hardware and I2C.
    // Returns false if the display does not respond.
    bool begin();

    // Show text (multi-line via '\n' possible).
    // No redraw if the text is unchanged.
    void show(const String& text);

    // Clear the display (without checking state).
    void clear();

private:
    uint8_t          _sda;
    uint8_t          _scl;
    uint8_t          _addr;
    Adafruit_SSD1306 _oled;
    String           _lastText;
};
