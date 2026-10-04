/*
 * ==============================================================================
 * Project: Intelligent Smart Locker & Anti-Theft Security System
 * Platform: Arduino UNO
 * Version: 1.0.0
 * Architecture: Finite State Machine (Non-blocking timing)
 * 
 * Hardware Modules:
 *  - Arduino UNO R3
 *  - 4x4 Membrane Matrix Keypad
 *  - Capacitive Touch Sensor (e.g., TTP223)
 *  - SG90 Micro Servo Motor
 *  - Analog / Digital Water Level Sensor
 *  - Green LED (Success / Verification Indicator)
 *  - Red LED (Alert / Tamper / Danger Indicator)
 *  - Active 5V Buzzer (Audible feedback & sirens)
 * 
 * Communication:
 *  - 9600 Baud USB Serial linked to Python voice assistant on Laptop
 * ==============================================================================
 */

#include <Servo.h>

#define NO_KEY '\0'

// ==========================================
// 1. PIN ALLOCATION DEFINITIONS
// ==========================================
// 4x4 Keypad Pin Mapping (Matched to physical ribbon cable orientation)
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
const byte ROW_PINS[KEYPAD_ROWS] = {2, 3, 4, 5}; // R1, R2, R3, R4 (Pins 1-4)
const byte COL_PINS[KEYPAD_COLS] = {6, 7, 8, 9}; // C1, C2, C3, C4 (Pins 5-8)

// Actuators & Outputs
const byte SERVO_PIN      = 10;  // SG90 Servo PWM signal
const byte BUZZER_PIN     = 11;  // Active 5V Buzzer
const byte TOUCH_PIN      = 12;  // Capacitive Touch Sensor (Digital Out)
const byte GREEN_LED_PIN  = A0;  // Green status LED
const byte RED_LED_PIN    = A1;  // Red alert LED

// Environmental Sensor
const byte WATER_PIN      = A3;  // Water Detection Sensor (Analog A3)

// ==========================================
// 2. KEYPAD MATRIX CONFIGURATION
// ==========================================
const char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ==========================================
// 3. ACTUATOR & SECURITY PARAMETERS
// ==========================================
Servo lockerServo;

// Servo Angles
const int SERVO_LOCKED_ANGLE = 0;    // Angle when locker bolt is locked
const int SERVO_OPEN_ANGLE   = 90;   // Angle when locker bolt is retracted

// Security PIN Configuration
const char DEFAULT_PIN[]       = "2580"; // Configurable standard access PIN
const char MASTER_RESET_PIN[]  = "9999"; // Admin reset PIN to clear lockouts
const byte MIN_PIN_LEN         = 4;      // Minimum digits required to submit (prevents ghost single-char lockout)
const byte MAX_PIN_LEN         = 8;
const byte MAX_ATTEMPTS        = 3;

// Touch Sensor Configuration (Auto-calibrated at boot to support both Active-HIGH and Active-LOW)
int touchIdleBaseline          = LOW;

// Water Emergency Sensor Configuration
const bool ENABLE_WATER_SENSOR = false;  // Disabled by default to prevent false buzzer alarms (set true to test water)
const int  WATER_THRESHOLD     = 700;    // Analog reading for immersion
const byte WATER_SAMPLES_REQUIRED = 4;

// Timers & Delays (Milliseconds)
const unsigned long UNLOCK_DURATION_MS     = 5000;  // How long locker stays open (5 sec)
const unsigned long PIN_ENTRY_TIMEOUT_MS   = 20000; // Timeout if user touches but abandons keypad (20 sec)

// ==========================================
// 4. SYSTEM STATES ENUMERATION
// ==========================================
enum SystemState {
  STATE_LOCKED,            // Normal secure state, bolt engaged
  STATE_HUMAN_VERIFIED,    // Touch verified, flashing green LED, alerting Python
  STATE_PIN_ENTRY,         // Accepting keypad digits
  STATE_ACCESS_GRANTED,    // Correct PIN entered, servo open, timer running
  STATE_WRONG_PIN,         // Invalid PIN attempt (< 3)
  STATE_INTRUSION,         // 3 failed PIN attempts (Lockout danger state)
  STATE_WATER_EMERGENCY    // Water/liquid tamper detected
};

// ==========================================
// 5. GLOBAL STATE VARIABLES
// ==========================================
SystemState currentState = STATE_LOCKED;
bool humanVerified = false;
byte passwordAttempts = 0;

// PIN Entry Buffer
char pinBuffer[MAX_PIN_LEN + 1];
byte pinIndex = 0;

// Timestamps for Non-blocking Operations
unsigned long unlockStartTime = 0;
unsigned long pinEntryStartTime = 0;
unsigned long lastAlarmToggleTime = 0;
bool alarmToggleState = false;

// Water Sensor Smoothing
byte waterConsecutiveHits = 0;

// Serial Command Buffer
String serialCommandBuffer = "";

// ==========================================
// 6. FUNCTION PROTOTYPES
// ==========================================
void sendEventToLaptop(const char* eventName);
bool readTouch();
char readKeypad();
void diagnoseKeypadHardware();
bool readWaterSensor();
void handleAuthentication();
void submitPin();
void handleSecurityState();
void unlockLocker();
void lockLocker();
void triggerIntrusionAlarm(const char* cause);
void triggerWaterAlarm();
void greenBlink(byte times, int onMs, int offMs);
void updateAlarmPeripherals();
void clearPinBuffer();
void resetSystem();
void handleSerialCommands();
void executeSerialCommand(String cmd);

// ==========================================
// 7. SETUP
// ==========================================
void setup() {
  // Initialize Serial Communication (Must match Python baud rate)
  Serial.begin(9600);
  while (!Serial && millis() < 1000) {
    // Brief wait for serial port initialization
  }

  // Pin Modes
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(WATER_PIN, INPUT);

  // Initial Hardware State
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  // Initialize Servo
  lockerServo.attach(SERVO_PIN);
  lockerServo.write(SERVO_LOCKED_ANGLE);
  delay(150);

  // Initialize Keypad Pins (All kept in INPUT_PULLUP to eliminate floating pin noise)
  for (byte r = 0; r < KEYPAD_ROWS; r++) {
    pinMode(ROW_PINS[r], INPUT_PULLUP);
  }
  for (byte c = 0; c < KEYPAD_COLS; c++) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }

  // Hardware Self-Test: Scan keypad matrix for any shorted jumper wires or stuck keys
  diagnoseKeypadHardware();

  // Visual & Audio Initialization Signal:
  // Both LEDs turn ON + one single short buzzer confirmation beep
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);                     // One time buzzer bip
  digitalWrite(BUZZER_PIN, LOW); // Buzzer silenced immediately
  delay(500);                    // LEDs remain ON to give a clear initialization signal
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW); // LEDs OFF -> System is armed and silent

  // TTP223 Touch Sensor Power-On Stabilization (TTP223 requires ~500ms after power-up)
  delay(200);

  // Auto-calibrate Touch Sensor Idle Baseline (10 samples over 100ms)
  int touchBaselineSum = 0;
  for (byte i = 0; i < 10; i++) {
    touchBaselineSum += digitalRead(TOUCH_PIN);
    delay(10);
  }
  touchIdleBaseline = (touchBaselineSum >= 5) ? HIGH : LOW;

  // Diagnostic Boot Telemetry
  Serial.println(F("========================================"));
  Serial.println(F("[BOOT] Arduino UNO Smart Locker Ready."));
  Serial.print(F("[BOOT] Default PIN: "));
  Serial.println(DEFAULT_PIN);
  Serial.print(F("[BOOT] Touch Baseline Calibrated: "));
  Serial.println(touchIdleBaseline == HIGH ? F("HIGH (Active-LOW sensor)") : F("LOW (Active-HIGH sensor)"));
  Serial.print(F("[BOOT] Water Emergency Alarm: "));
  Serial.println(ENABLE_WATER_SENSOR ? F("ENABLED") : F("DISABLED (Prevents false beeps)"));
  Serial.println(F("[BOOT] System Armed -> State: LOCKED (Silent & Ready)"));
  Serial.println(F("========================================"));

  // Notify Python Voice Agent
  sendEventToLaptop("SYSTEM_STARTED");

  // Initial secure state
  currentState = STATE_LOCKED;
  humanVerified = false;
  passwordAttempts = 0;
  clearPinBuffer();
}

// ==========================================
// 8. MAIN LOOP (STATE MACHINE)
// ==========================================
void loop() {
  // Always check incoming Serial commands from Python laptop
  handleSerialCommands();

  // Non-blocking alarm siren & warning LED update
  updateAlarmPeripherals();

  // Priority 1: Check Environmental / Water Emergency
  if (currentState != STATE_INTRUSION && currentState != STATE_WATER_EMERGENCY) {
    if (readWaterSensor()) {
      triggerWaterAlarm();
      return;
    }
  }

  // Priority 2: Execute State-Specific Security Logic
  handleSecurityState();
}

// ==========================================
// 9. SECURITY STATE MACHINE HANDLER
// ==========================================
void handleSecurityState() {
  switch (currentState) {

    case STATE_LOCKED: {
      // System is locked. Check for touch presence first.
      if (readTouch()) {
        humanVerified = true;
        currentState = STATE_HUMAN_VERIFIED;
        return;
      }

      // If user presses keypad before touching the touch sensor:
      char key = readKeypad();
      if (key != NO_KEY) {
        static unsigned long lastBlockedAlert = 0;
        if (millis() - lastBlockedAlert > 2000) { // Rate limit: at most once every 2.0s
          lastBlockedAlert = millis();
          sendEventToLaptop("KEYPAD_BLOCKED");
          Serial.print(F("[INFO] Keypad pressed before touch! Key: '"));
          Serial.print(key);
          Serial.println(F("' rejected. Please touch sensor first."));
          
          // Single brief warning flash on Red LED & Buzzer
          digitalWrite(RED_LED_PIN, HIGH);
          digitalWrite(BUZZER_PIN, HIGH);
          delay(40);
          digitalWrite(BUZZER_PIN, LOW);
          digitalWrite(RED_LED_PIN, LOW);
        }
      }
      break;
    }

    case STATE_HUMAN_VERIFIED: {
      // Touch-based human-presence verification confirmed!
      sendEventToLaptop("HUMAN_DETECTED");

      // Visual feedback: Green LED blinks 3 times
      greenBlink(3, 120, 100);

      // Transition into PIN entry mode and start timeout timer
      pinEntryStartTime = millis();
      clearPinBuffer();

      // Brief 150ms pause to ensure touch electrostatic charge is completely dissipated
      delay(150);

      currentState = STATE_PIN_ENTRY;
      break;
    }

    case STATE_PIN_ENTRY: {
      // Timeout check: revert to locked if user touched but walked away
      if (millis() - pinEntryStartTime > PIN_ENTRY_TIMEOUT_MS) {
        humanVerified = false;
        clearPinBuffer();
        currentState = STATE_LOCKED;
        sendEventToLaptop("LOCKER_LOCKED");
        return;
      }

      // Check for keypad input
      handleAuthentication();
      break;
    }

    case STATE_ACCESS_GRANTED: {
      // Locker is open! Maintain open state until duration expires.
      if (millis() - unlockStartTime >= UNLOCK_DURATION_MS) {
        lockLocker();
        currentState = STATE_LOCKED;
        humanVerified = false;
      }
      break;
    }

    case STATE_WRONG_PIN: {
      // Wrong PIN event already signaled; return to PIN_ENTRY if attempts < 3
      if (passwordAttempts >= MAX_ATTEMPTS) {
        triggerIntrusionAlarm("THREE_ATTEMPTS");
      } else {
        clearPinBuffer();
        pinEntryStartTime = millis(); // Refresh timeout for next try
        currentState = STATE_PIN_ENTRY;
      }
      break;
    }

    case STATE_INTRUSION:
    case STATE_WATER_EMERGENCY: {
      // Alarm lockout state. Locker remains strictly locked.
      // Master PIN override directly on keypad: "9999#"
      char key = readKeypad();
      if (key != NO_KEY) {
        if (key == '#') {
          if (strcmp(pinBuffer, MASTER_RESET_PIN) == 0) {
            resetSystem();
          } else {
            clearPinBuffer();
          }
        } else if (key == '*') {
          clearPinBuffer();
        } else {
          if (pinIndex < MAX_PIN_LEN) {
            pinBuffer[pinIndex++] = key;
            pinBuffer[pinIndex] = '\0';
          }
        }
      }
      break;
    }
  }
}

// ==========================================
// ==========================================
// 10. AUTHENTICATION & KEYPAD LOGIC
// ==========================================

/*
 * Verifies entered PIN buffer against DEFAULT_PIN ("2580").
 * Called automatically when 4 digits are entered, or manually on '#' / 'D'.
 */
void submitPin() {
  if (pinIndex < MIN_PIN_LEN) {
    Serial.print(F("[KEYPAD] PIN too short ("));
    Serial.print(pinIndex);
    Serial.print(F(" digits: \""));
    Serial.print(pinBuffer);
    Serial.println(F("\"). Must be at least 4 digits! Attempt NOT penalized."));
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  Serial.print(F("[AUTH] Submitting PIN: \""));
  Serial.print(pinBuffer);
  Serial.println(F("\" for verification..."));

  // Verify entered PIN against DEFAULT_PIN ("2580")
  if (strcmp(pinBuffer, DEFAULT_PIN) == 0) {
    // PIN MATCH!
    Serial.println(F("[AUTH] PIN MATCH! Access Granted."));
    passwordAttempts = 0;
    clearPinBuffer();
    
    // Play confirmation beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);

    // Open locker servo to 90 degrees & send event to Python voice
    unlockLocker();
    currentState = STATE_ACCESS_GRANTED;
  } else {
    // PIN MISMATCH!
    passwordAttempts++;
    Serial.print(F("[AUTH] INCORRECT PIN! Failed Attempts: "));
    Serial.print(passwordAttempts);
    Serial.print(F(" / "));
    Serial.println(MAX_ATTEMPTS);

    clearPinBuffer();
    sendEventToLaptop("WRONG_PASSWORD");

    // Warning beep & double red flash
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    delay(100);
    digitalWrite(RED_LED_PIN, HIGH);
    delay(100);
    digitalWrite(RED_LED_PIN, LOW);

    currentState = STATE_WRONG_PIN;
  }
}

void handleAuthentication() {
  char key = readKeypad();
  if (key == NO_KEY) {
    return;
  }

  // Refresh entry timer on each keystroke
  pinEntryStartTime = millis();

  // Keypad Feedback Beep (Crisp 15ms chirp)
  digitalWrite(BUZZER_PIN, HIGH);
  delay(15);
  digitalWrite(BUZZER_PIN, LOW);

  if (key == '*') {
    // Clear / Backspace function
    clearPinBuffer();
    Serial.println(F("[KEYPAD] '*' pressed -> PIN Buffer Cleared."));
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  if (key == '#' || key == 'D') {
    // Manual Enter / Confirm function
    submitPin();
    return;
  }

  // Normal digit entry (0-9, A, B, C)
  if (pinIndex < MAX_PIN_LEN) {
    pinBuffer[pinIndex++] = key;
    pinBuffer[pinIndex] = '\0';
    Serial.print(F("[KEYPAD] Key: '"));
    Serial.print(key);
    Serial.print(F("' | Current PIN Buffer: \""));
    Serial.print(pinBuffer);
    Serial.println(F("\""));

    // AUTO-SUBMIT: When 4 digits are entered, immediately verify & unlock!
    if (pinIndex == 4) {
      delay(80); // Brief visual feedback delay before submit
      submitPin();
      return;
    }
  }
}

// ==========================================
// 11. SENSOR READERS & FILTERS
// ==========================================

/*
 * Hardware Self-Test: Scans the entire 4x4 matrix for any short-circuits or stuck keys.
 * Runs once at boot before the system is armed.
 */
void diagnoseKeypadHardware() {
  Serial.println(F("[DIAGNOSTIC] Scanning 4x4 Keypad Matrix for short-circuits or stuck keys..."));
  bool faultFound = false;

  for (byte c = 0; c < KEYPAD_COLS; c++) {
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], LOW);
    delayMicroseconds(50);

    for (byte r = 0; r < KEYPAD_ROWS; r++) {
      pinMode(ROW_PINS[r], INPUT_PULLUP);
      delayMicroseconds(50);
      if (digitalRead(ROW_PINS[r]) == LOW) {
        faultFound = true;
        char stuck = keys[r][c];
        Serial.print(F("  [!] HARDWARE ALERT: Key '"));
        Serial.print(stuck);
        Serial.print(F("' is CLOSED/SHORTED at startup! (Row Pin D"));
        Serial.print(ROW_PINS[r]);
        Serial.print(F(" <--> Col Pin D"));
        Serial.print(COL_PINS[c]);
        Serial.println(F(")"));
      }
    }
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }

  // Restore all row pins to INPUT_PULLUP for Keypad library
  for (byte r = 0; r < KEYPAD_ROWS; r++) {
    pinMode(ROW_PINS[r], INPUT_PULLUP);
  }
  for (byte c = 0; c < KEYPAD_COLS; c++) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }

  if (faultFound) {
    Serial.println(F("  -------------------------------------------------------------"));
    Serial.println(F("  [FIX GUIDE] If you did NOT press the keypad:"));
    Serial.println(F("   1. Inspect Arduino D9 & D5 wires (ensure they are not touching)."));
    Serial.println(F("   2. Press/massage button '1' on the keypad to release stuck membrane dome."));
    Serial.println(F("  -------------------------------------------------------------"));
  } else {
    Serial.println(F("  [OK] Keypad Matrix Healthy: All 16 keys open and ready!"));
  }
}

/*
 * Native 4x4 Matrix Keypad Scanner & Debouncer:
 * Built specifically for both Arduino UNO R3 (AVR) & Arduino UNO R4 (Renesas RA4M1 48MHz):
 * 1. Pull-up Protected: Columns and Rows are ALWAYS held in INPUT_PULLUP (never floating!).
 * 2. 25us settling delay ensures membrane ribbon capacitance discharges cleanly at 48MHz.
 * 3. 50us double-check confirmation eliminates transient EMI spikes.
 * 4. Strict Single-Press Enforcement: Returns a key once per physical press.
 *    Requires complete key release before the same key can register again.
 *    Completely eliminates phantom machine-gun flooding!
 */
char readKeypad() {
  // Ensure all row pins are pulled HIGH
  for (byte r = 0; r < KEYPAD_ROWS; r++) {
    pinMode(ROW_PINS[r], INPUT_PULLUP);
  }
  // Ensure all column pins are pulled HIGH
  for (byte c = 0; c < KEYPAD_COLS; c++) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }

  char rawKey = NO_KEY;

  for (byte c = 0; c < KEYPAD_COLS; c++) {
    // Drive this single column LOW
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], LOW);

    // Renesas RA4M1 (UNO R4 @ 48MHz) line-settling delay
    delayMicroseconds(25);

    for (byte r = 0; r < KEYPAD_ROWS; r++) {
      if (digitalRead(ROW_PINS[r]) == LOW) {
        // True physical debouncing: A real human finger press lasts > 20ms.
        // Electrostatic transients and touch-sensor EMI last < 1ms.
        delay(15);
        if (digitalRead(ROW_PINS[r]) == LOW) {
          rawKey = keys[r][c];
          break;
        }
      }
    }

    // Restore column back to INPUT_PULLUP (HIGH)
    digitalWrite(COL_PINS[c], HIGH);
    pinMode(COL_PINS[c], INPUT_PULLUP);

    if (rawKey != NO_KEY) {
      break;
    }
  }

  // Single-Press Enforcement & Full Release Tracker
  static char lastReportedKey = NO_KEY;
  static unsigned long lastValidPressTime = 0;
  unsigned long now = millis();

  // If no button is currently touching contacts:
  if (rawKey == NO_KEY) {
    lastReportedKey = NO_KEY; // Armed for next physical press
    return NO_KEY;
  }

  // If a button is held down continuously, DO NOT repeat!
  if (rawKey == lastReportedKey) {
    return NO_KEY;
  }

  // Rate limiter: Minimum 150ms between distinct key presses
  if (now - lastValidPressTime < 150) {
    return NO_KEY;
  }

  // Confirmed new physical press
  lastReportedKey = rawKey;
  lastValidPressTime = now;
  return rawKey;
}

/*
 * Reads Capacitive Touch Sensor with Auto-Calibrated Baseline & State Transition.
 * Works with both Active-HIGH and Active-LOW modules without manual configuration!
 * Only triggers on clean transition away from baseline, sustained for 30ms.
 */
bool touchWasActive = false;

bool readTouch() {
  // Grace period: ignore touch sensor for first 2 seconds after boot
  if (millis() < 2000) {
    return false;
  }

  int rawState = digitalRead(TOUCH_PIN);

  // If sensor is at resting idle baseline
  if (rawState == touchIdleBaseline) {
    touchWasActive = false; // Arm for next touch
    return false;
  }

  // If already triggered for this touch, wait until user lifts finger
  if (touchWasActive) {
    return false;
  }

  // Sustained 80ms confirmation (4 samples of 20ms)
  // Eliminates 100% of electrostatic spikes and power rail fluctuations!
  for (byte i = 0; i < 4; i++) {
    delay(20);
    if (digitalRead(TOUCH_PIN) == touchIdleBaseline) {
      return false; // Noise glitch rejected!
    }
  }

  // Confirmed genuine physical human touch event!
  touchWasActive = true;
  Serial.println(F("[TOUCH] Human presence verified! Switching to PIN entry."));
  return true;
}


/*
 * Reads Water Sensor on Analog Pin A3.
 * Only active if ENABLE_WATER_SENSOR is true (preventing false buzzer beeps).
 */
bool readWaterSensor() {
  if (!ENABLE_WATER_SENSOR) {
    return false;
  }

  // Grace period: ignore water sensor for first 3 seconds after boot
  if (millis() < 3000) {
    return false;
  }

  static unsigned long lastWaterCheck = 0;
  unsigned long now = millis();

  // Sample every 80ms
  if (now - lastWaterCheck >= 80) {
    lastWaterCheck = now;
    int waterVal = analogRead(WATER_PIN);

    if (waterVal >= WATER_THRESHOLD) {
      waterConsecutiveHits++;
      if (waterConsecutiveHits >= WATER_SAMPLES_REQUIRED) {
        Serial.print(F("[DEBUG ALERT] Water sensor triggered! Analog raw value: "));
        Serial.println(waterVal);
        return true; // Confirmed water exposure emergency
      }
    } else {
      waterConsecutiveHits = 0; // Reset consecutive hit counter
    }
  }
  return false;
}

// ==========================================
// 12. ACTUATORS & PERIPHERALS CONTROLLER
// ==========================================

void unlockLocker() {
  lockerServo.write(SERVO_OPEN_ANGLE);
  digitalWrite(GREEN_LED_PIN, HIGH); // Green LED ON while unlocked
  sendEventToLaptop("LOCKER_OPEN");
  unlockStartTime = millis();
}

void lockLocker() {
  lockerServo.write(SERVO_LOCKED_ANGLE);
  digitalWrite(GREEN_LED_PIN, LOW); // Green LED OFF
  sendEventToLaptop("LOCKER_LOCKED");
}

void greenBlink(byte times, int onMs, int offMs) {
  for (byte i = 0; i < times; i++) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(GREEN_LED_PIN, LOW);
    if (i < times - 1) {
      delay(offMs);
    }
  }
}

void triggerIntrusionAlarm(const char* cause) {
  currentState = STATE_INTRUSION;
  lockerServo.write(SERVO_LOCKED_ANGLE); // Ensure servo remains locked
  digitalWrite(GREEN_LED_PIN, LOW);
  sendEventToLaptop(cause);
}

void triggerWaterAlarm() {
  currentState = STATE_WATER_EMERGENCY;
  lockerServo.write(SERVO_LOCKED_ANGLE); // Ensure servo remains locked
  digitalWrite(GREEN_LED_PIN, LOW);
  sendEventToLaptop("WATER_DANGER");
}

/*
 * Non-blocking alarm siren & LED flasher for INTRUSION and WATER_EMERGENCY.
 * Allows CPU to continue checking serial commands and master PIN keypad input.
 */
void updateAlarmPeripherals() {
  if (currentState == STATE_INTRUSION || currentState == STATE_WATER_EMERGENCY) {
    unsigned long now = millis();
    int interval = (currentState == STATE_WATER_EMERGENCY) ? 120 : 180; // Fast siren for water

    if (now - lastAlarmToggleTime >= interval) {
      lastAlarmToggleTime = now;
      alarmToggleState = !alarmToggleState;
      digitalWrite(RED_LED_PIN, alarmToggleState ? HIGH : LOW);
      digitalWrite(BUZZER_PIN, alarmToggleState ? HIGH : LOW);
    }
  }
}

void clearPinBuffer() {
  memset(pinBuffer, 0, sizeof(pinBuffer));
  pinIndex = 0;
}

/*
 * Safe reset procedure: Clears all alarm states, stops buzzer/LEDs,
 * re-engages lock, resets attempts counter, and returns to STATE_LOCKED.
 */
void resetSystem() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  lockerServo.write(SERVO_LOCKED_ANGLE);

  currentState = STATE_LOCKED;
  humanVerified = false;
  passwordAttempts = 0;
  clearPinBuffer();

  sendEventToLaptop("ALARM_RESET");
  sendEventToLaptop("LOCKER_LOCKED");
}

// ==========================================
// 13. SERIAL PROTOCOL & PC COMMUNICATION
// ==========================================

void sendEventToLaptop(const char* eventName) {
  Serial.println(eventName);
}

/*
 * Processes incoming ASCII commands from Python over USB Serial.
 * Supported commands:
 *  - "RESET" or "ALARM_RESET": Safely clears lockout / danger alarm
 *  - "UNLOCK": Admin remote unlock
 *  - "LOCK": Admin remote lock
 *  - "STATUS": Returns current system state & attempts
 */
void handleSerialCommands() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      serialCommandBuffer.trim();
      if (serialCommandBuffer.length() > 0) {
        executeSerialCommand(serialCommandBuffer);
        serialCommandBuffer = "";
      }
    } else {
      if (serialCommandBuffer.length() < 30) {
        serialCommandBuffer += c;
      }
    }
  }
}

void executeSerialCommand(String cmd) {
  cmd.toUpperCase();
  if (cmd == "RESET" || cmd == "ALARM_RESET") {
    resetSystem();
  } else if (cmd == "INIT" || cmd == "HELLO" || cmd == "START") {
    sendEventToLaptop("SYSTEM_STARTED");
  } else if (cmd == "UNLOCK") {
    unlockLocker();
    currentState = STATE_ACCESS_GRANTED;
  } else if (cmd == "LOCK") {
    lockLocker();
    currentState = STATE_LOCKED;
    humanVerified = false;
  } else if (cmd == "STATUS") {
    Serial.print(F("STATE:"));
    switch (currentState) {
      case STATE_LOCKED:         Serial.print(F("LOCKED")); break;
      case STATE_HUMAN_VERIFIED: Serial.print(F("HUMAN_VERIFIED")); break;
      case STATE_PIN_ENTRY:      Serial.print(F("PIN_ENTRY")); break;
      case STATE_ACCESS_GRANTED: Serial.print(F("ACCESS_GRANTED")); break;
      case STATE_WRONG_PIN:      Serial.print(F("WRONG_PIN")); break;
      case STATE_INTRUSION:      Serial.print(F("INTRUSION")); break;
      case STATE_WATER_EMERGENCY:Serial.print(F("WATER_EMERGENCY")); break;
    }
    Serial.print(F(" | ATTEMPTS:"));
    Serial.println(passwordAttempts);
  }
}
