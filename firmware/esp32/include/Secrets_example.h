// ============================================================
//  Secrets.example.h
//  Template for Secrets.h.
//
//  Copy this file to Secrets.h and fill in the real
//  credentials:
//
//      cp firmware/esp32/include/Secrets.example.h \
//         firmware/esp32/include/Secrets.h
//
//  Secrets.h itself is listed in .gitignore and is NOT
//  version-controlled. Only this example file belongs in the
//  repo.
// ============================================================

#pragma once

namespace Config {
    namespace Wifi {
        constexpr char SSID[]     = "CHANGE_ME";
        constexpr char PASSWORD[] = "CHANGE_ME_TOO";
    }
}
