# KIRBY – Autonomous Mobile Robot

An experimental robotics project based on the **ESP32 DevKit V1**. The goal is not fixed – the project evolves as new ideas and hardware are explored.

---

## Hardware

### Core Platform

| Component       | Details                                                          |
| --------------- | ---------------------------------------------------------------- |
| Microcontroller | ESP32 DevKit V1 (30-pin, 240 MHz dual-core, Wi-Fi/BT)            |
| Motor driver    | L298N (dual H-bridge); channel A = left, channel B = right       |
| Motors          | 4× TT Gear Motor 6 V (2 per channel, parallel)                   |
| Display         | SSD1306 OLED 128×64 (I2C, address `0x3C`)                        |
| Level shifter   | Logic Level Converter (5 V → 3.3 V) for the three HC-SR04 ECHO lines |
| Chassis         | 4WD TT-Motor chassis                                             |

Motor PWM: 2 kHz, 8 bit (0–255), LEDC channels 0 (A) and 1 (B).

### Sensors

| Sensor      | Quantity | Mounting                                                | Function                                                                 |
| ----------- | -------- | ------------------------------------------------------- | ------------------------------------------------------------------------ |
| HC-SR04     | 3×       | Front centre, front left (angled), front right (angled) | Obstacle distance measurement (valid range 1–200 cm)                     |
| TCRT5000 IR | 1×       | Middle (downward)                                       | Lift / floor detection → motor stop (emergency stop, **disabled by default**, see below) |

### Power System

| Component  | Details                                                |
| ---------- | ------------------------------------------------------ |
| Battery    | Keeppower 2S1P 18650 7.4 V / 3400 mAh (integrated BMS) |
| Charger    | Enerpower Plug Charger 2S (8.4 V CC/CV, 2 A)           |
| 5 V rail   | MP1584EN step-down → Sensors + Display                 |
| 3.3 V rail | MP1584EN step-down → ESP32                             |

---

## Pin Assignment (30-Pin ESP32)

### Side A – VIN (Sensors)

| GPIO | Label | Function                                         |
| ---- | ----- | ------------------------------------------------ |
| 27   | D27   | HC-SR04 centre – TRIG                            |
| 14   | D14   | HC-SR04 centre – ECHO *(Logic Level Converter!)* |
| 26   | D26   | HC-SR04 left – TRIG                              |
| 25   | D25   | HC-SR04 left – ECHO *(Logic Level Converter!)*   |
| 33   | D33   | HC-SR04 right – TRIG                             |
| 32   | D32   | HC-SR04 right – ECHO *(Logic Level Converter!)*  |

### Side B – 3V3 (Motor control + I2C)

| GPIO | Label | Function                |
| ---- | ----- | ----------------------- |
| 4    | D4    | IN1 – Motor A direction |
| 16   | RX2   | IN2 – Motor A direction |
| 17   | TX2   | IN3 – Motor B direction |
| 5    | D5    | IN4 – Motor B direction |
| 18   | D18   | ENA – Motor A PWM       |
| 19   | D19   | ENB – Motor B PWM       |
| 21   | D21   | SDA – SSD1306 I2C       |
| 22   | D22   | SCL – SSD1306 I2C       |
| 13   | D13   | IR floor bottom         |

> **Notes:**
>
> - GPIO 12 (D12) is a strapping pin – not used
> - GPIO 2 (D2) and GPIO 15 (D15) are strapping pins – not used
> - D5 has a pull-up at boot (HIGH); safe as IN4 because ENB (D19) stays LOW at boot → Motor B remains disabled

---

## Software

### Build System

- PlatformIO (Arduino framework), configured in the `platformio.ini` in the repository root
- Serial monitor: 115200 baud

### Setup

Wi-Fi credentials are kept out of version control. Before building:

```
cp firmware/esp32/include/Secrets.example.h firmware/esp32/include/Secrets.h
```

Then edit `Secrets.h` and set `SSID` and `PASSWORD`. These are the credentials of the **access point the robot creates** (not of an existing network). The password must be at least 8 characters (WPA2). `Secrets.h` is listed in `.gitignore` and must never be committed.

### Libraries

| Library            | Purpose                                      |
| ------------------ | -------------------------------------------- |
| `Adafruit SSD1306` (^2.5.9)        | OLED display                 |
| `Adafruit GFX Library` (^1.11.9)   | Graphics base layer          |
| `WiFi.h`           | ESP32 core (built-in)                        |
| `WebServer.h`      | ESP32 core (built-in)                        |
| `Wire.h`           | ESP32 core (built-in), I2C                   |

The HC-SR04 sensors are driven directly via `pulseIn()` – no external library.

### Repository Structure

```
KIRBY/
│
├── firmware/
│   └── esp32/
│       ├── include/
│       │   ├── Config.h            # All pins, thresholds and driving parameters
│       │   ├── Secrets.example.h   # Template – committed
│       │   └── Secrets.h           # Real credentials – gitignored, not committed
│       └── src/
│           ├── drivers/            # Hardware abstractions: MotorDriver, UltrasonicSensor,
│           │                       #   IrSensor, Display
│           ├── navigation/         # Navigation: state machine, avoidance routines
│           ├── robot/              # Top-level Robot class (INIT / READY / RUN)
│           ├── wifi/               # WiFiController.h/.cpp – Access Point + HTTP endpoints
│           │                       # WebUI.h – embedded HTML/CSS/JS interface
│           └── main.cpp
│
├── .gitignore
├── platformio.ini
└── README.md
```

---

## Navigation Logic

All sensors are read every 80 ms (two samples per HC-SR04, averaged; invalid readings are discarded). While driving, a priority-based reactive state machine is evaluated every loop iteration:

```
1. IR floor bottom          →  Only if EMERGENCY_STOP_ENABLED (default: false):
                                immediate stop while lifted; once the floor is
                                detected again, wait SAFETY_DELAY_MS, then resume
2. Any HC-SR04 ≤ STOP       →  Stop → reverse for REV_DURATION at the selected
                                speed → pivot in place toward the freer side
                                (tie: right) until all three sensors read more
                                than STOP + MARGIN_CM → resume
3. Centre ≤ FRONT_TURN      →  Curve toward the freer side (tie: right)
4. Left ≤ SIDE_TURN         →  Curve right
   Right ≤ SIDE_TURN        →  Curve left
5. Otherwise                →  Straight ahead
```

**Speed:** The speed depends on the centre sensor only. At or above `FRONT_TURN` the robot drives at the selected speed level; between `FRONT_TURN` and `STOP` it is reduced linearly down to the speed of level 1; at or below `STOP` it uses the speed of level 1. An invalid centre reading (timeout/out of range) is treated cautiously and also limits the speed to level 1. For the obstacle checks themselves, an invalid reading counts as "free".

**Curves:** In a curve the outer wheel runs at the current speed, the inner wheel at `speed × TURN_FACTOR / 100`. A negative turn factor makes the inner wheel run backwards for a tighter turn.

**Stuck handling:** If no direction becomes free within `SCAN_TURN_TIMEOUT_MS` while pivoting, the robot stops, shows "Kein Weg frei" and returns to normal driving instead of spinning indefinitely.

**Emergency stop:** The safety delay (`SAFETY_DELAY_MS`) always starts counting from the moment the robot is set back down, not from the moment it was picked up – so the wait is the same length regardless of how long it was held in the air. The feature is switched off by default; set `EMERGENCY_STOP_ENABLED = true` in `Config.h` to enable it.

### Speed Levels and Default Parameters

Each speed level has its own set of defaults in `Config.h`:

| Parameter                  | Level 1 | Level 2 | Level 3 |
| -------------------------- | ------- | ------- | ------- |
| Speed (PWM 0–255)          | 165     | 205     | 245     |
| Front turn threshold (cm)  | 50      | 65      | 70      |
| Side turn threshold (cm)   | 40      | 45      | 45      |
| Stop threshold (cm)        | 15      | 17      | 18      |
| Turn factor (%)            | 10      | 5       | 1       |
| Reverse duration (ms)      | 400     | 350     | 300     |

> **Note:** Parameters changed in the Web UI apply at runtime only. Selecting a different speed level loads that level's defaults and overwrites any runtime-adjusted values. The values are still being tuned.

---

## Web Interface

Access point: SSID/password from `Secrets.h` (Wi-Fi channel 1, max. 1 client) · Interface: `http://192.168.4.1`

- Start/stop the autonomous drive
- 3 speed levels
- Adjustable parameters: front turn threshold (cm), side threshold (cm), stop threshold (cm), turn factor (%), reverse duration (ms) – the input fields are populated live from `/status` and kept in sync with the firmware's actual values (not hardcoded), so they never drift from `Config.h` or from values changed at runtime. A field is not overwritten while it has focus.
- Live status every 800 ms: robot state and left/centre/right distances
- Layout adapts to portrait and landscape orientation

---

## Current Status

Experimental project – no fixed goal. Current implementation covers basic reactive navigation with three-sensor obstacle detection, curve steering, three speed levels and a Wi-Fi/Web-UI control. The IR-based lift stop is implemented but disabled by default.