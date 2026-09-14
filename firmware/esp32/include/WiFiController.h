// ============================================================
//  WiFiController.h
//  Manages the WiFi access point and HTTP web server.
//
//  This class knows neither Robot nor Navigation directly –
//  commands are forwarded via registered callbacks.
//  This avoids circular dependencies.
//
//  Robot registers all callbacks at startup; after that,
//  it's enough to call handle() regularly in loop().
// ============================================================

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <functional>

class WiFiController {
public:
    // --------------------------------------------------------
    //  Callback types
    // --------------------------------------------------------
    using VoidCb    = std::function<void()>;
    using IntCb     = std::function<void(int)>;
    using StringCb  = std::function<String()>;
    using ParamCb   = std::function<void(int turn, int sideTurn, int turnFactor)>;

    // --------------------------------------------------------
    //  Constructor
    // --------------------------------------------------------
    WiFiController(const char* ssid,
                   const char* password,
                   uint8_t     channel    = 1,
                   bool        hidden     = false,
                   uint8_t     maxClients = 1);

    // Start the access point and register routes
    void begin();

    // Process web server requests (call on every loop() iteration)
    void handle();

    // --------------------------------------------------------
    //  Register callbacks (call before begin())
    // --------------------------------------------------------
    void onStart    (VoidCb   cb) { _cbStart  = cb; }
    void onStop     (VoidCb   cb) { _cbStop   = cb; }
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
    IntCb     _cbStufe;
    ParamCb   _cbParam;
    StringCb  _cbStatus;

    void _setupRoutes();
};
