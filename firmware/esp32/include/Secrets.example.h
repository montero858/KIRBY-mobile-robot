// ============================================================
//  Secrets.example.h
//  Vorlage fuer Secrets.h.
//
//  Diese Datei nach Secrets.h kopieren und die echten
//  Zugangsdaten eintragen:
//
//      cp firmware/esp32/include/Secrets.example.h \
//         firmware/esp32/include/Secrets.h
//
//  Secrets.h selbst steht in .gitignore und wird NICHT
//  versioniert. Nur diese Beispieldatei gehoert ins Repo.
// ============================================================

#pragma once

namespace Config {
    namespace Wifi {
        constexpr char SSID[]     = "CHANGE_ME";
        constexpr char PASSWORD[] = "CHANGE_ME_TOO";
    }
}
