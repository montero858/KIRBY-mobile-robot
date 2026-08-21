# KIRBY – Autonomous Mobile Robot

An experimental robotics project based on the **ESP32 DevKit V1**. The goal is not fixed – the project evolves as new ideas and hardware are explored.

---

## Hardware

### Core Platform

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 DevKit V1 (30-pin, 240 MHz dual-core, Wi-Fi/BT) |
| Motor driver | L298N (dual H-bridge) |
| Motors | 4× TT Gear Motor 6 V (2 per channel, parallel) |
| Display | SSD1306 OLED 128×64 (I2C) |
| Chassis | 4WD TT-Motor chassis |

### Sensors

| Sensor | Quantity | Mounting | Function |
|--------|----------|----------|----------|
| HC-SR04 | 3× | Front centre, front left (angled), front right (angled) | Obstacle distance measurement |
| TCRT5000 IR | 1× | Middle (downward) | Motor-Stop |

### Power System

| Component | Details |
|-----------|---------|
| Battery | Keeppower 2S1P 18650 7.4 V / 3400 mAh (integrated BMS) |
| Charger | Enerpower Plug Charger 2S (8.4 V CC/CV, 2 A) |
| 5 V rail | MP1584EN step-down → Sensors + Display |
| 3.3 V rail | MP1584EN step-down → ESP32 |

---

## Pin Assignment (30-Pin ESP32)

### Side A – VIN (Sensors)

| GPIO | Label | Function |
|------|-------|----------|
| 27 | D27 | HC-SR04 centre – TRIG |
| 14 | D14 | HC-SR04 centre – ECHO *(Logic Level Converter!)* |
| 26 | D26 | HC-SR04 left – TRIG |
| 25 | D25 | HC-SR04 left – ECHO *(Logic Level Converter!)* |
| 33 | D33 | HC-SR04 right – TRIG |
| 32 | D32 | HC-SR04 right – ECHO *(Logic Level Converter!)* |


### Side B – 3V3 (Motor control + I2C)

| GPIO | Label | Function |
|------|-------|----------|
| 4 | D4 | IN1 – Motor A direction |
| 16 | RX2 | IN2 – Motor A direction |
| 17 | TX2 | IN3 – Motor B direction |
| 5 | D5 | IN4 – Motor B direction |
| 18 | D18 | ENA – Motor A PWM |
| 19 | D19 | ENB – Motor B PWM |
| 21 | D21 | SDA – SSD1306 I2C |
| 22 | D22 | SCL – SSD1306 I2C |
| 13 | D13 | IR floor bottom |

> **Notes:**
> - GPIO 12 (D12) is a strapping pin – not used
> - GPIO 2 (D2) and GPIO 15 (D15) are strapping pins – not used
> - D5 has a pull-up at boot (HIGH); safe as IN4 because ENB (D19) stays LOW at boot → Motor B remains disabled

---

## Software

### Build System

- PlatformIO (Arduino framework)

### Setup

WiFi credentials are kept out of version control. Before building:

```
cp firmware/esp32/include/Secrets.example.h firmware/esp32/include/Secrets.h
```

Then edit `Secrets.h` and fill in the real `SSID`/`PASSWORD`. This file is listed in `.gitignore` and must never be committed.

### Libraries

| Library | Purpose |
|---------|---------|
| `Adafruit SSD1306` | OLED display |
| `Adafruit GFX` | Graphics base layer |
| `WiFi.h` | ESP32 core (built-in) |
| `WebServer.h` | ESP32 core (built-in) |

### Repository Structure

```
KIRBY/
│
├── docs/                        # Wiring diagrams, pin tables, notes
│
├── firmware/
│   └── esp32/
│       ├── include/             # Shared headers
│       ├── src/
│       │   ├── drivers/         # Hardware abstractions: Motor, Ultrasonic, IR, Display
│       │   ├── navigation/      # State machine, scan logic, avoidance routines
│       │   ├── robot/           # Top-level Robot class
│       │   ├── wifi/            # WiFiController.h/.cpp – Access Point + HTTP endpoints
│       │   │                    # WebUI.h – embedded HTML/CSS/JS interface
|       |   ├── Config.h
│       │   ├── Secrets.example.h   # Template – committed
│       │   ├── Secrets.h           # Real credentials – gitignored, not committed
│       │   └── main.cpp
│       └── platformio.ini
│
├── .gitignore
└── README.md
```

---

## Navigation Logic

Priority-based reactive state machine evaluated every loop iteration:

```
1. IR floor bottom        →  Immediate stop while lifted; once the sensor
                              clears (robot back on the ground), waits a
                              fixed safety delay before resuming
2. Any HC-SR04 < STOP     →  Stop → reverse briefly → turn toward the freer
                              side (based on last left/right readings) → resume
3. HC-SR04 centre < SLOW  →  Reduce speed linearly toward the minimum
4. Otherwise              →  Forward at the selected speed level
```

Speed reduces linearly between the `FRONT_TURN_THRESHOLD_CM` and the `STOP_THRESHOLD_CM`. Below `STOP_THRESHOLD_CM`, the robot halts and scans by comparing left and right readings to choose the freer side. If neither side clears within `SCAN_TURN_TIMEOUT_MS`, the robot gives up turning and returns to normal driving rather than spinning indefinitely.

The safety delay after being lifted (`SAFETY_DELAY_MS`) always starts counting from the moment the robot is set back down, not from the moment it was picked up — so the wait is the same length regardless of how long it was held in the air.

---

## Web Interface

Access point: **KIRBY** · Interface: `http://192.168.4.1`

- Start/stop the autonomous drive
- 4 speed levels
- Adjustable turn and stop thresholds (cm) — input fields are populated live from `/status` on load and kept in sync with the firmware's actual values (not hardcoded), so they never drift from `Config.h` or from values changed at runtime
- Live status (distances, state) every 800 ms
- Layout adapts to portrait and landscape orientation

---

## Current Status

Experimental project – no fixed goal. Current implementation covers basic reactive navigation with three-sensor obstacle detection and IR-based safety stop.