// ============================================================
//  WiFiController.h
//  Verwaltet WLAN-Access-Point und HTTP-Webserver.
//
//  Die Klasse kennt weder Robot noch Navigation direkt –
//  Befehle werden ueber registrierte Callbacks weitergeleitet.
//  So entstehen keine zirkulaeren Abhaengigkeiten.
//
//  Robot registriert beim Start alle Callbacks, danach
//  genuegt der regelmaessige Aufruf von handle() in loop().
// ============================================================

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <functional>

class WiFiController {
public:
    // --------------------------------------------------------
    //  Callback-Typen
    // --------------------------------------------------------
    using VoidCb    = std::function<void()>;
    using IntCb     = std::function<void(int)>;
    using StringCb  = std::function<String()>;
    using ParamCb   = std::function<void(int slow, int stop, int turn)>;

    // --------------------------------------------------------
    //  Konstruktor
    // --------------------------------------------------------
    WiFiController(const char* ssid,
                   const char* password,
                   uint8_t     channel    = 1,
                   bool        hidden     = false,
                   uint8_t     maxClients = 1);

    // Access Point starten und Routen registrieren
    void begin();

    // Webserver-Anfragen abarbeiten (jeden loop()-Durchlauf aufrufen)
    void handle();

    // --------------------------------------------------------
    //  Callbacks registrieren (vor begin() aufrufen)
    // --------------------------------------------------------
    void onStart    (VoidCb   cb) { _cbStart  = cb; }
    void onStop     (VoidCb   cb) { _cbStop   = cb; }
    void onScan     (VoidCb   cb) { _cbScan   = cb; }
    void onStufe    (IntCb    cb) { _cbStufe  = cb; }
    void onParameter(ParamCb  cb) { _cbParam  = cb; }
    void onStatus   (StringCb cb) { _cbStatus = cb; }

    String ipAddress() const { return WiFi.softAPIP().toString(); }

private:
    const char* _ssid;
    const char* _password;
    uint8_t     _channel;
    bool        _hidden;
    uint8_t     _maxClients;
    WebServer   _server;

    VoidCb    _cbStart;
    VoidCb    _cbStop;
    VoidCb    _cbScan;
    IntCb     _cbStufe;
    ParamCb   _cbParam;
    StringCb  _cbStatus;

    void _setupRoutes();
};
