// ============================================================
//  WiFiController.cpp
// ============================================================

#include "WiFiController.h"
#include "WebUI.h"
#include "Config.h"

// ============================================================
//  Constructor
// ============================================================

WiFiController::WiFiController(const char* ssid,
                               const char* password,
                               uint8_t     channel,
                               bool        hidden,
                               uint8_t     maxClients)
    : _ssid(ssid),
      _password(password),
      _channel(channel),
      _hidden(hidden),
      _maxClients(maxClients),
      _server(80)
{}

// ============================================================
//  begin()
// ============================================================

void WiFiController::begin() {
    WiFi.softAP(_ssid, _password, _channel, _hidden, _maxClients);
    Serial.print(F("[WiFi] AP gestartet: "));
    Serial.println(ipAddress());
    _setupRoutes();
    _server.begin();
}

void WiFiController::handle() {
    _server.handleClient();
}

// ============================================================
//  _setupRoutes()
//  Wire up all HTTP endpoints with callbacks.
// ============================================================

void WiFiController::_setupRoutes() {

    _server.on("/", [this]() {
        _server.send_P(200, "text/html", HTML_SEITE);
    });

    _server.on("/start", [this]() {
        if (_cbStart) _cbStart();
        _server.send(200, "text/plain", "Roboter faehrt");
    });

    _server.on("/stopp", [this]() {
        if (_cbStop) _cbStop();
        _server.send(200, "text/plain", "Gestoppt");
    });

    _server.on("/stufe", [this]() {
        if (!_server.hasArg("n")) {
            _server.send(400, "text/plain", "Fehlender Parameter: n");
            return;
        }
        int stufe = _server.arg("n").toInt();
        if (stufe < 1 || stufe > 4) {
            _server.send(400, "text/plain", "Ungueltige Stufe (1-4)");
            return;
        }
        if (_cbStufe) _cbStufe(stufe);
        _server.send(200, "text/plain", "Stufe " + String(stufe) + " aktiv");
    });

    _server.on("/parameter", [this]() {
        int turn = _server.hasArg("turn")
                   ? _server.arg("turn").toInt() : PARAM_UNSET;
        int sideTurn = _server.hasArg("sideTurn")
                       ? _server.arg("sideTurn").toInt() : PARAM_UNSET;
        int stop = _server.hasArg("stop")
                   ? _server.arg("stop").toInt() : PARAM_UNSET;
        // toInt() handles a leading "-" correctly, so negative
        // turnFactor values arrive here as proper negative ints.
        int turnFactor = _server.hasArg("turnFactor")
                         ? _server.arg("turnFactor").toInt() : PARAM_UNSET;
        int reverseDuration = _server.hasArg("revDuration")
                              ? _server.arg("revDuration").toInt() : PARAM_UNSET;

        if (_cbParam) _cbParam(turn, sideTurn, stop, turnFactor, reverseDuration);
        _server.send(200, "text/plain", "Parameter gesetzt");
    });

    _server.on("/status", [this]() {
        String status = _cbStatus ? _cbStatus() : "–";
        _server.send(200, "text/plain", status);
    });
}