// ============================================================
//  Display.cpp
// ============================================================

#include "Display.h"

Display::Display(uint8_t sda, uint8_t scl,
                 uint8_t i2cAddr, uint8_t width, uint8_t height)
    : _sda(sda),
      _scl(scl),
      _addr(i2cAddr),
      _oled(width, height, &Wire, -1),
      _lastText("")
{}

bool Display::begin() {
    Wire.begin(_sda, _scl);
    if (!_oled.begin(SSD1306_SWITCHCAPVCC, _addr)) {
        Serial.println(F("[Display] Init fehlgeschlagen"));
        return false;
    }
    _oled.clearDisplay();
    _oled.display();
    return true;
}

void Display::show(const String& text) {
    if (text == _lastText) return;
    _lastText = text;

    Serial.println(text);
    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setCursor(0, 0);
    _oled.print(text.c_str());
    _oled.display();
}

void Display::clear() {
    _lastText = "";
    _oled.clearDisplay();
    _oled.display();
}
