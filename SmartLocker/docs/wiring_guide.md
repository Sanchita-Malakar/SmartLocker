# 🔌 Step-by-Step Hardware Wiring & Pinout Guide
### Intelligent Smart Locker & Anti-Theft Security System

This guide walks you through connecting every single pin of the Smart Locker system to your **Arduino UNO R3** using a solderless breadboard.

---

## 1. Complete Pin-to-Pin Master Map

| Component | Pin Name / Lead | Connect To | Wire Color Suggestion | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **Power Rails** | Arduino 5V | Breadboard (+) Red Rail | Red | Main 5V power bus |
| | Arduino GND | Breadboard (-) Blue Rail | Black | Common circuit ground |
| **TTP223 Touch** | VCC | Breadboard (+) Red Rail | Red | 5V Sensor power |
| | GND | Breadboard (-) Blue Rail | Black | Sensor ground |
| | SIG / I/O | **Arduino D12** | Yellow | Touch presence signal |
| **Green LED** | Long leg (Anode +) | Resistor 220Ω $\rightarrow$ **Arduino A0** | Green | Verified / Unlock indicator |
| | Short leg (Cathode -) | Breadboard (-) Blue Rail | Black | Ground |
| **Red LED** | Long leg (Anode +) | Resistor 220Ω $\rightarrow$ **Arduino A1** | Red | Warning / Danger indicator |
| | Short leg (Cathode -) | Breadboard (-) Blue Rail | Black | Ground |
| **Active Buzzer** | (+) Anode (Long leg / +) | **Arduino D11** | Blue | Audio feedback & alarm siren |
| | (-) Cathode (Short leg) | Breadboard (-) Blue Rail | Black | Ground |
| **SG90 Servo** | Brown wire | Breadboard (-) Blue Rail | Brown/Black | Servo ground |
| | Red wire | Breadboard (+) Red Rail | Red | Servo 5V power |
| | Orange/Yellow wire | **Arduino D10** | Orange/Yellow | PWM signal control |
| **4×4 Keypad** | Pin 1 (Row 1) | **Arduino D2** | Brown | Row 1: `1, 2, 3, A` |
| | Pin 2 (Row 2) | **Arduino D3** | Red | Row 2: `4, 5, 6, B` |
| | Pin 3 (Row 3) | **Arduino D4** | Orange | Row 3: `7, 8, 9, C` |
| | Pin 4 (Row 4) | **Arduino D5** | Yellow | Row 4: `*, 0, #, D` |
| | Pin 5 (Col 1) | **Arduino D6** | Green | Col 1: `1, 4, 7, *` |
| | Pin 6 (Col 2) | **Arduino D7** | Blue | Col 2: `2, 5, 8, 0` |
| | Pin 7 (Col 3) | **Arduino D8** | Purple | Col 3: `3, 6, 9, #` |
| | Pin 8 (Col 4) | **Arduino D9** | Gray | Col 4: `A, B, C, D` |
| **Water Sensor** | (+) / VCC | Breadboard (+) Red Rail | Red | 5V module power |
| | (-) / GND | Breadboard (-) Blue Rail | Black | Module ground |
| | S / SIG | **Arduino A3** (Pin 17) | Green | Analog moisture level signal |

---

## 2. Step-by-Step Connection Instructions

### Step 1: Establish the Common Power Bus
1. Take a **Red jumper wire** from the Arduino **5V** header and plug it into the breadboard's **(+) Red power rail**.
2. Take a **Black jumper wire** from the Arduino **GND** header and plug it into the breadboard's **(-) Blue ground rail**.
> *All sensor VCC and GND pins will now connect to these two rails.*

### Step 2: Connect the TTP223 Capacitive Touch Sensor
The module has 3 pins: `VCC`, `GND`, and `SIG` (or `I/O`).
1. Connect **VCC** to the Breadboard **(+) Red Rail**.
2. Connect **GND** to the Breadboard **(-) Blue Rail**.
3. Connect **SIG** to **Arduino Digital Pin D12**.

### Step 3: Connect the Status LEDs (Green & Red)
Each LED has two legs:
- **Long leg = Anode (+)**
- **Short leg = Cathode (-)** (also has a flat notch on the plastic rim)

**Green LED (Verification & Unlock indicator):**
1. Insert the Green LED into two empty breadboard rows.
2. Connect a **220Ω resistor** from the **Long leg (+)** row to **Arduino Pin A0**.
3. Connect a jumper wire from the **Short leg (-)** row to the Breadboard **(-) Blue Rail**.

**Red LED (Tamper & Intrusion indicator):**
1. Insert the Red LED into two empty breadboard rows.
2. Connect a **220Ω resistor** from the **Long leg (+)** row to **Arduino Pin A1**.
3. Connect a jumper wire from the **Short leg (-)** row to the Breadboard **(-) Blue Rail**.

### Step 4: Connect the Active 5V Buzzer
The active buzzer has polarity markings:
1. Connect the **(+) Positive pin** (longer leg or marked with `+` on top) to **Arduino Digital Pin D11**.
2. Connect the **(-) Negative pin** (shorter leg) to the Breadboard **(-) Blue Rail**.

### Step 5: Connect the SG90 Micro Servo
The servo has a 3-pin female connector with color-coded wires:
- **Brown (or Black):** Ground $\rightarrow$ Connect to Breadboard **(-) Blue Rail**.
- **Red:** 5V Power $\rightarrow$ Connect to Breadboard **(+) Red Rail**.
- **Orange (or Yellow):** PWM Signal $\rightarrow$ Connect to **Arduino Digital Pin D10**.

### Step 6: Connect the 4×4 Matrix Membrane Keypad
The membrane keypad has an 8-pin ribbon connector. Holding the keypad facing you with the ribbon pointing down, the pins from **Left to Right** are numbered **1 to 8**:
- **Pins 1 to 4 are the Rows (R1, R2, R3, R4):**
  - Pin 1 (R1) $\rightarrow$ **Arduino Pin D2**
  - Pin 2 (R2) $\rightarrow$ **Arduino Pin D3**
  - Pin 3 (R3) $\rightarrow$ **Arduino Pin D4**
  - Pin 4 (R4) $\rightarrow$ **Arduino Pin D5**
- **Pins 5 to 8 are the Columns (C1, C2, C3, C4):**
  - Pin 5 (C1) $\rightarrow$ **Arduino Pin D6**
  - Pin 6 (C2) $\rightarrow$ **Arduino Pin D7**
  - Pin 7 (C3) $\rightarrow$ **Arduino Pin D8**
  - Pin 8 (C4) $\rightarrow$ **Arduino Pin D9**

### Step 7: Connect the Water Level Sensor
The water sensor has 3 pins: `+` (VCC), `-` (GND), and `S` (Signal).
1. Connect **+** to Breadboard **(+) Red Rail**.
2. Connect **-** to Breadboard **(-) Blue Rail**.
3. Connect **S (SIG)** to **Arduino Analog Pin A3**.

---

## 3. Pre-Power Checklist

Before plugging in the USB cable to your laptop:
- [ ] Ensure **5V and GND** are never connected directly to each other (short circuit check).
- [ ] Confirm the **LEDs have 220Ω resistors** attached to their positive legs.
- [ ] Check the **SG90 Servo wire colors**: Red is 5V, Brown is GND, Orange is D10.
- [ ] Confirm **Arduino D0 and D1** have NO wires connected (they are dedicated to USB Serial for the Python voice assistant).
