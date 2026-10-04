# 🔐 Intelligent Smart Locker & Anti-Theft Security System

An intelligent, state-machine-driven physical locker security and anti-theft defense system built with **Arduino UNO**, multimodal sensors, dual visual indicators, an active acoustic siren, and an offline **Python Voice Alert Assistant** running on your host laptop.

---

## 📑 Table of Contents

- [1. System Highlights](#1-system-highlights)
- [2. Hardware Bill of Materials (BOM)](#2-hardware-bill-of-materials-bom)
- [3. Pin Connections & Wiring Diagram](#3-pin-connections--wiring-diagram)
- [4. Software Installation & Setup](#4-software-installation--setup)
  - [4.1 Arduino Firmware Setup](#41-arduino-firmware-setup)
  - [4.2 Python Voice Assistant Setup](#42-python-voice-assistant-setup)
- [5. Step-by-Step 11-Phase Testing Guide](#5-step-by-step-11-phase-testing-guide)
- [6. Operation Manual & Voice Map](#6-operation-manual--voice-map)
- [7. Sensor Calibration & Troubleshooting](#7-sensor-calibration--troubleshooting)
- [8. Repository Architecture](#8-repository-architecture)

---

## 1. System Highlights

- **Two-Stage Human-Presence Authentication:** The keypad remains locked and completely unresponsive to PIN entry attempts until the capacitive touch sensor registers verified human touch.
- **Finite State Machine Architecture:** Eliminates spaghetti logic with clean, deterministic states (`STATE_LOCKED`, `STATE_HUMAN_VERIFIED`, `STATE_PIN_ENTRY`, `STATE_ACCESS_GRANTED`, `STATE_WRONG_PIN`, `STATE_INTRUSION`, `STATE_WATER_EMERGENCY`).
- **Three-Attempt Lockout Defense:** Automatically locks the system into danger mode if 3 consecutive invalid PINs are entered.
- **Liquid Tamper & Emergency Defense:** Water sensor with analog multi-sample confirmation detects liquid attacks or environmental hazards.
- **Offline Vocal Warnings:** Uses Python (`pyttsx3`) over USB Serial—no internet or cloud API required.
- **Safe Reset Protocols:** Lockout alarms can be safely disarmed from the Python console (`reset`) or via the keypad admin code (`9999#`).

---

## 2. Hardware Bill of Materials (BOM)

| Component | Quantity | Role |
| :--- | :--- | :--- |
| **Arduino UNO R3** | 1 | Central controller executing the state machine |
| **4×4 Matrix Membrane Keypad** | 1 | PIN code input mechanism |
| **TTP223 Capacitive Touch Sensor** | 1 | Touch-based human-presence verification |
| **SG90 Micro Servo Motor** | 1 | Mechanical deadbolt locking mechanism |
| **Water Level Sensor Module** | 1 | Liquid tamper and emergency alert detection |
| **Green LED (5mm)** | 1 | Visual indicator for verified touch & unlock state |
| **Red LED (5mm)** | 1 | Visual indicator for warnings, tamper, & alarms |
| **Active 5V Buzzer** | 1 | Acoustic feedback and loud alarm sirens |
| **Resistors (220Ω – 330Ω)** | 2 | Current-limiting resistors for LEDs |
| **Solderless Breadboard & Jumpers**| 1 | Circuit assembly and power distribution |
| **USB Type-A to Type-B Cable** | 1 | Power and bidirectional serial link to laptop |

---

## 3. Pin Connections & Wiring Diagram

### 3.1 Arduino UNO Pinout Table

```
========================================================================================
ARDUINO PIN       CONNECTED MODULE PIN        PURPOSE & NOTES
========================================================================================
D0 (RX) / D1 (TX) Reserved for USB Serial     DO NOT CONNECT TO BREADBOARD
D2                Keypad Row 1    (Pin 1)     Keypad '1', '2', '3', 'A'
D3                Keypad Row 2    (Pin 2)     Keypad '4', '5', '6', 'B'
D4                Keypad Row 3    (Pin 3)     Keypad '7', '8', '9', 'C'
D5                Keypad Row 4    (Pin 4)     Keypad '*', '0', '#', 'D'
D6                Keypad Column 1 (Pin 5)     Keypad '1', '4', '7', '*'
D7                Keypad Column 2 (Pin 6)     Keypad '2', '5', '8', '0'
D8                Keypad Column 3 (Pin 7)     Keypad '3', '6', '9', '#'
D9                Keypad Column 4 (Pin 8)     Keypad 'A', 'B', 'C', 'D'
D10 (PWM)         SG90 Servo (Orange/Yellow)  PWM Signal control wire
D11               Active Buzzer (+)           Long leg / positive terminal
D12               Touch Sensor (SIG / I/O)    Digital output from TTP223
A0 (Pin 14)       Green LED (+) Anode         In series with 220Ω resistor to A0
A1 (Pin 15)       Red LED (+) Anode           In series with 220Ω resistor to A1
A2 (Pin 16)       Unassigned / Free           Reserved for future expansion
A3 (Pin 17)       Water Sensor (SIG / S)      Analog output from moisture trace grid
5V                Breadboard (+) Red Rail     Supplies 5V to all sensors & servo
GND               Breadboard (-) Blue Rail    Common GND for all modules & LED cathodes
========================================================================================
```

---

## 4. Software Installation & Setup

### 4.1 Arduino Firmware Setup

1. Open the **Arduino IDE** (v1.8.x or v2.x).
2. Install the **Keypad** library:
   - Go to **Tools** $\rightarrow$ **Manage Libraries...**
   - In the search bar, type `Keypad`
   - Locate **Keypad by Mark Stanley, Alexander Brevig** and click **Install**.
3. Open `SmartLocker/Arduino/SmartLocker/SmartLocker.ino`.
4. Connect your Arduino UNO to your computer using the USB cable.
5. In the Arduino IDE:
   - Select **Tools** $\rightarrow$ **Board** $\rightarrow$ **Arduino Uno**.
   - Select **Tools** $\rightarrow$ **Port** $\rightarrow$ (Select your Arduino COM port, e.g., `COM3` or `COM4`).
6. Click **Upload** (Arrow icon).
7. You will hear a short startup chirp from the buzzer, and the green/red LEDs will remain off. The locker is now armed.

### 4.2 Python Voice Assistant Setup

The Python application provides offline voice announcements and an administrative control shell.

1. Open PowerShell or Command Prompt in the project folder:
   ```powershell
   cd "d:\Embedded system\SmartLocker\Python"
   ```
2. Install dependencies:
   ```powershell
   pip install -r requirements.txt
   ```
3. Run the application:
   ```powershell
   python security_voice.py
   ```
4. The script will automatically detect your Arduino COM port and initialize the voice synthesizer.

---

## 5. Step-by-Step Testing Guide

To ensure smooth hardware validation without troubleshooting multiple components at once, follow this systematic order:

```
Phase 1: Arduino + Green LED
   ↓
Phase 2: Touch Sensor + Green LED ×3
   ↓
Phase 3: Touch Sensor + USB Serial + Laptop Voice ("Accurately human detected")
   ↓
Phase 4: 4×4 Keypad + Serial Telemetry (Keypad blocked before touch)
   ↓
Phase 5: Keypad + PIN Authentication + 3 Attempt Lockout
   ↓
Phase 6: SG90 Micro Servo LOCK / OPEN Angles
   ↓
Phase 7: Full Normal Cycle (Touch → PIN → Servo OPEN → 5s Delay → Servo LOCK)
   ↓
Phase 8: Water Sensor Emergency + Red LED + High-Pitch Siren + Voice ("Throwing water...")
   ↓
Phase 9: Complete Integration & State Transitions
   ↓
Phase 10: Production Hardening & Safe Master Reset
```

### Detailed Phase Instructions

- **Phase 1 (Green LED):** Flash the sketch or verify that the Green LED lights up during boot/test.
- **Phase 2 (Touch $\rightarrow$ 3 Blinks):** Touch the TTP223 sensor pad. Verify that the Green LED blinks exactly 3 times.
- **Phase 3 (Laptop Voice):** Run `python security_voice.py`. Touch the sensor. The laptop speaks: *"Accurately human detected."*
- **Phase 4 (Keypad Before Touch):** Without touching the touch sensor, press any key on the keypad. The laptop announces: *"Please ensure you are human."* The system rejects the entry.
- **Phase 5 (PIN Entry):** Touch the sensor, then enter `2580#`. Observe the buzzer confirmation beep and laptop announcement: *"Access granted."* Next, test an invalid PIN (`1234#`). Notice the laptop announces *"Invalid password."* After 3 consecutive wrong PINs, notice the system enters `STATE_INTRUSION` and speaks: *"Invalid user. Danger signal is detected."*
- **Phase 6 & 7 (Servo Operation):** Enter `2580#`. Verify the SG90 servo sweeps from $0^\circ$ to $90^\circ$ (Open), stays open for 5 seconds with the Green LED illuminated, and then smoothly sweeps back to $0^\circ$ (Locked).
- **Phase 8 (Water Tamper):** Place a drop of water or moist cloth on the water sensor grid—the system triggers `WATER_EMERGENCY`, initiates the rapid siren, and speaks: *"Throwing water... Danger Danger."*
- **Phase 9 & 10 (Safe Reset):** Disarm any alarm state by typing `reset` in the Python console or by entering `9999#` on the keypad. The alarm shuts off, and the system re-locks.

---

## 6. Operation Manual & Voice Map

### Default Security Credentials

- **Default User Access PIN:** `2580` (Followed by `#` to confirm)
- **Clear / Backspace Key:** `*`
- **Master Admin Reset Code:** `9999#` (Disarms active alarms and clears lockouts)

### Voice Alert Matrix

| Event Signal | Laptop Voice Output | Hardware Actions |
| :--- | :--- | :--- |
| `SYSTEM_STARTED` | *"Smart locker system initialized and secured."* | Servo locked at $0^\circ$, all LEDs off |
| `HUMAN_DETECTED` | *"Accurately human detected."* | Green LED blinks 3 times, opens 20s PIN window |
| `KEYPAD_BLOCKED` | *"Please ensure you are human."* | Red LED blips, keypad entry ignored |
| `WRONG_PASSWORD` | *"Invalid password."* | Red LED double flash, warning chirp |
| `ACCESS_GRANTED` | *"Access granted."* | Green LED solid ON, servo rotates to $90^\circ$ |
| `LOCKER_LOCKED` | *"Locker locked."* | Servo returns to $0^\circ$, Green LED OFF |
| `THREE_ATTEMPTS` | *"Invalid user. Danger signal is detected."* | Red strobe, alarm siren, system lockout |
| `WATER_DANGER` | *"Throwing water... Danger Danger."* | Fast red strobe, rapid siren, system lockout |
| `ALARM_RESET` | *"Security alarm reset. System is now locked."* | Alarm silenced, servo locked, returns to normal |

---

## 7. Sensor Calibration & Troubleshooting

### Water Sensor Verification
- The water sensor uses an analog threshold of `350` (range 0 to 1023).
- Dry sensor: Reads $\approx 0 - 80$.
- Finger touching traces: Reads $\approx 100 - 200$.
- Water drop / immersion: Reads $> 400$.
- If your environment requires adjustment, update `WATER_THRESHOLD` in `SmartLocker.ino`.

### Servo Power Fluctuations
- If your Arduino resets or freezes when the servo turns, the servo is pulling peak current. Place a **$100\ \mu\text{F} - 470\ \mu\text{F}$ capacitor** between 5V and GND on your breadboard to buffer transient surges.

---

## 8. Repository Architecture

```
SmartLocker/
│
├── Arduino/
│   └── SmartLocker.ino            # Complete Arduino UNO firmware & FSM
│
├── Python/
│   ├── security_voice.py          # Multithreaded voice assistant & control shell
│   ├── requirements.txt           # Python library requirements
│   └── security_events.log        # Automatically generated audit trail
│
├── docs/
│   └── system_architecture.md     # Detailed state diagrams, pinouts & algorithms
│
└── README.md                      # Comprehensive project guide & testing manual
```
