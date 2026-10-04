"""
==============================================================================
Project: Intelligent Smart Locker & Anti-Theft Security System
Module: Laptop Voice Assistant & Security Monitor
Platform: Python 3.8+ (Windows 10 / 11 Native SAPI + pyttsx3 fallback)
==============================================================================
"""

import sys
import os
import time
import queue
import threading
import datetime
from pathlib import Path
import serial
import serial.tools.list_ports

# Ensure UTF-8 output on Windows CMD to prevent encoding crashes
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

# ==========================================
# 1. EVENT TO VOICE MAPPING DICTIONARY
# ==========================================
EVENT_VOICE_MAP = {
    "SYSTEM_STARTED": "Smart locker system activated and secured. Please ensure you are human by touching the sensor.",
    "HUMAN_DETECTED": "Human detected successfully. Please enter the password now.",
    "KEYPAD_BLOCKED": "Please ensure you are human by touching the sensor.",
    "WRONG_PASSWORD": "Invalid password.",
    "ACCESS_GRANTED": "Password matched. Unlocked locker.",
    "LOCKER_OPEN": "Password matched. Unlocked locker.",
    "LOCKER_LOCKED": "Locker locked.",
    "WATER_DANGER": "Throwing water... Danger Danger.",
    "THREE_ATTEMPTS": "Invalid user. Danger signal is detected.",
    "ALARM_RESET": "Security alarm reset. System is now locked."
}

# Terminal ANSI Styling
class Colors:
    RESET   = "\033[0m"
    BOLD    = "\033[1m"
    RED     = "\033[91m"
    GREEN   = "\033[92m"
    YELLOW  = "\033[93m"
    BLUE    = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN    = "\033[96m"
    WHITE   = "\033[97m"

LOG_FILE = Path(__file__).parent / "security_events.log"

# ==========================================
# 2. BULLET-PROOF NATIVE VOICE WORKER
# ==========================================
class VoiceWorker(threading.Thread):
    """
    Dedicated worker thread for offline speech synthesis.
    Primary engine: Windows Native SAPI (direct, zero lag, max volume)
    Fallback engine: pyttsx3
    """
    def __init__(self, speech_queue: queue.Queue):
        super().__init__(daemon=True)
        self.speech_queue = speech_queue
        self.running = True

    def run(self):
        # 1. Try Windows Native SAPI (Most reliable on Windows 10/11)
        sapi_voice = None
        try:
            import win32com.client
            import pythoncom
            pythoncom.CoInitialize()
            sapi_voice = win32com.client.Dispatch("SAPI.SpVoice")
            sapi_voice.Volume = 100
            sapi_voice.Rate = 0
        except Exception:
            sapi_voice = None

        # 2. Prepare pyttsx3 as secondary fallback
        pyttsx3_engine = None
        if not sapi_voice:
            try:
                import pyttsx3
                pyttsx3_engine = pyttsx3.init()
                pyttsx3_engine.setProperty("volume", 1.0)
                pyttsx3_engine.setProperty("rate", 160)
            except Exception:
                pyttsx3_engine = None

        while self.running:
            try:
                text = self.speech_queue.get(timeout=0.5)
                if text is None:
                    break

                spoken = False

                # Strategy A: Windows SAPI
                if sapi_voice:
                    try:
                        # 0 = synchronous in worker thread (sequential speech queue)
                        sapi_voice.Speak(text, 0)
                        spoken = True
                    except Exception:
                        spoken = False

                # Strategy B: pyttsx3
                if not spoken and pyttsx3_engine:
                    try:
                        pyttsx3_engine.say(text)
                        pyttsx3_engine.runAndWait()
                        spoken = True
                    except Exception:
                        spoken = False

                # Strategy C: Windows PowerShell System.Speech fallback
                if not spoken:
                    try:
                        import subprocess
                        escaped = text.replace('"', '`"')
                        cmd = f'Add-Type -AssemblyName System.Speech; [System.Speech.Synthesis.SpeechSynthesizer]::new().Speak("{escaped}")'
                        subprocess.run(["powershell", "-NoProfile", "-Command", cmd], timeout=8)
                        spoken = True
                    except Exception:
                        pass

                self.speech_queue.task_done()
            except queue.Empty:
                continue
            except Exception as e:
                print(f"\n{Colors.RED}[TTS Error]{Colors.RESET} {e}")

    def stop(self):
        self.running = False
        self.speech_queue.put(None)


# ==========================================
# 3. SECURITY MONITOR APPLICATION
# ==========================================
class SmartLockerMonitor:
    def __init__(self, port: str = None, baudrate: int = 9600):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.running = True
        self.last_spoken_text = ""
        self.last_spoken_time = 0.0
        self.speech_queue = queue.Queue()
        self.voice_worker = VoiceWorker(self.speech_queue)
        self.voice_worker.start()

    def speak(self, text: str):
        """Enqueue speech utterance non-blockingly with anti-spam duplicate cooldown."""
        now = time.time()
        if text == self.last_spoken_text and (now - self.last_spoken_time) < 3.0:
            return  # Suppress repetitive back-to-back voice spam
        self.last_spoken_text = text
        self.last_spoken_time = now
        self.speech_queue.put(text)

    def log(self, event_name: str, voice_text: str = ""):
        """Log event with ISO timestamp to console and file."""
        now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Color coding by severity
        if "DANGER" in event_name or "INTRUSION" in event_name or "THREE" in event_name:
            tag_color = Colors.RED
        elif "GRANTED" in event_name or "HUMAN" in event_name or "OPEN" in event_name:
            tag_color = Colors.GREEN
        elif "BLOCKED" in event_name or "WRONG" in event_name:
            tag_color = Colors.YELLOW
        else:
            tag_color = Colors.CYAN

        console_msg = f"{Colors.WHITE}[{now_str}]{Colors.RESET} {tag_color}{Colors.BOLD}{event_name:<20}{Colors.RESET}"
        if voice_text:
            console_msg += f" -> \"{voice_text}\""
        print(console_msg)

        # Append to persistent log file
        try:
            with open(LOG_FILE, "a", encoding="utf-8") as f:
                f.write(f"[{now_str}] {event_name:<20} {voice_text}\n")
        except Exception:
            pass

    @staticmethod
    def detect_arduino_port():
        """Scans connected serial ports and attempts auto-detection."""
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            return None

        # Look for keywords common to Arduino or USB-to-UART bridges
        keywords = ["arduino", "ch340", "cp210", "ftdi", "usb serial", "usb-serial"]
        for p in ports:
            desc = (p.description or "").lower()
            mfg = (p.manufacturer or "").lower()
            if any(k in desc or k in mfg for k in keywords):
                return p.device

        # If only one port exists, return it
        if len(ports) == 1:
            return ports[0].device

        return None

    @staticmethod
    def list_all_ports():
        """Lists all active COM ports."""
        ports = list(serial.tools.list_ports.comports())
        return [(p.device, p.description) for p in ports]

    def connect(self) -> bool:
        """Establish serial connection with Arduino."""
        if not self.port:
            self.port = self.detect_arduino_port()

        if not self.port:
            available = self.list_all_ports()
            if not available:
                print(f"{Colors.YELLOW}[!] No serial COM ports detected. Please plug in the Arduino UNO via USB.{Colors.RESET}")
                return False

            print(f"\n{Colors.YELLOW}[?] Select your Arduino COM port:{Colors.RESET}")
            for idx, (dev, desc) in enumerate(available, 1):
                print(f"   [{idx}] {dev} - {desc}")
            try:
                choice = input(f"{Colors.BOLD}Select port number [1-{len(available)}]: {Colors.RESET}").strip()
                selected_idx = int(choice) - 1
                if 0 <= selected_idx < len(available):
                    self.port = available[selected_idx][0]
                else:
                    self.port = available[0][0]
            except Exception:
                self.port = available[0][0]

        print(f"{Colors.CYAN}[*] Connecting to Arduino on {self.port} at {self.baudrate} baud...{Colors.RESET}")
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=1.0)
            time.sleep(2.0)  # Allow Arduino DTR reset recovery
            print(f"{Colors.GREEN}[OK] Serial connection established successfully on {self.port}!{Colors.RESET}")
            # Send initialization handshake so Arduino broadcasts SYSTEM_STARTED
            self.send_command("INIT")
            return True
        except Exception as e:
            print(f"{Colors.RED}[!] Failed to open serial port {self.port}: {e}{Colors.RESET}")
            self.serial_conn = None
            return False

    def send_command(self, cmd: str):
        """Send command string to Arduino."""
        if self.serial_conn and self.serial_conn.is_open:
            try:
                line = (cmd.strip() + "\n").encode("utf-8")
                self.serial_conn.write(line)
                self.serial_conn.flush()
                print(f"{Colors.MAGENTA}[TX Command]{Colors.RESET} Sent to Arduino: {cmd.upper()}")
            except Exception as e:
                print(f"{Colors.RED}[TX Error]{Colors.RESET} {e}")
        else:
            print(f"{Colors.RED}[!] Serial connection is not active.{Colors.RESET}")

    def serial_listener(self):
        """Continuously reads incoming events from Arduino."""
        while self.running:
            if not self.serial_conn or not self.serial_conn.is_open:
                time.sleep(1.0)
                continue

            try:
                raw_line = self.serial_conn.readline()
                if not raw_line:
                    continue

                line = raw_line.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                # Parse event
                if line in EVENT_VOICE_MAP:
                    voice_msg = EVENT_VOICE_MAP[line]
                    self.log(line, voice_msg)
                    self.speak(voice_msg)
                elif line.startswith("STATE:"):
                    print(f"{Colors.CYAN}[SYSTEM TELEMETRY]{Colors.RESET} {line}")
                elif line.startswith("["):
                    # Debug or boot telemetry from Arduino firmware
                    print(f"{Colors.WHITE}{line}{Colors.RESET}")
                else:
                    self.log(line, "")

            except serial.SerialException as se:
                print(f"\n{Colors.RED}[!] Serial disconnect detected: {se}{Colors.RESET}")
                try:
                    self.serial_conn.close()
                except Exception:
                    pass
                self.serial_conn = None
                print(f"{Colors.YELLOW}[*] Attempting to reconnect every 3 seconds...{Colors.RESET}")
                while self.running and not self.serial_conn:
                    time.sleep(3)
                    if self.connect():
                        break
            except Exception as ex:
                if self.running:
                    print(f"\n{Colors.RED}[Listener Error]{Colors.RESET} {ex}")
                time.sleep(0.1)

    def interactive_shell(self):
        """CLI control console for testing and administration."""
        print("\n" + "=" * 65)
        print(f"{Colors.BOLD}{Colors.CYAN}  [SMART LOCKER VOICE & SECURITY MONITOR CONSOLE]{Colors.RESET}")
        print("=" * 65)
        print("Commands:")
        print(f"  {Colors.BOLD}status{Colors.RESET}     - Query system state & failed attempts from Arduino")
        print(f"  {Colors.BOLD}reset{Colors.RESET}      - Safely disarm & reset danger/intrusion alarm")
        print(f"  {Colors.BOLD}unlock{Colors.RESET}     - Admin override: open locker servo")
        print(f"  {Colors.BOLD}lock{Colors.RESET}       - Admin override: lock locker servo")
        print(f"  {Colors.BOLD}test [evt]{Colors.RESET} - Test voice alert without triggering hardware")
        print(f"  {Colors.BOLD}events{Colors.RESET}     - List all recognizable voice events")
        print(f"  {Colors.BOLD}quit / exit{Colors.RESET}- Exit monitor application")
        print("=" * 65 + "\n")

        # In standalone mode or if Arduino hasn't broadcasted startup yet, announce greeting
        if not self.serial_conn or self.last_spoken_time == 0.0:
            startup_phrase = EVENT_VOICE_MAP.get("SYSTEM_STARTED", "Smart locker system activated and secured. Please ensure you are human by touching the sensor.")
            self.log("SYSTEM_STARTED", startup_phrase)
            self.speak(startup_phrase)

        while self.running:
            try:
                cmd_line = input(f"{Colors.BOLD}SmartLocker > {Colors.RESET}").strip()
                if not cmd_line:
                    continue

                parts = cmd_line.split(maxsplit=1)
                verb = parts[0].lower()

                if verb in ["quit", "exit"]:
                    print(f"{Colors.YELLOW}[*] Shutting down monitor...{Colors.RESET}")
                    self.running = False
                    break
                elif verb == "status":
                    self.send_command("STATUS")
                elif verb == "reset":
                    self.send_command("RESET")
                elif verb in ["unlock", "open"]:
                    self.send_command("UNLOCK")
                elif verb in ["lock", "close"]:
                    self.send_command("LOCK")
                elif verb == "events":
                    print(f"\n{Colors.CYAN}Supported Arduino Events & Voice Phrases:{Colors.RESET}")
                    for evt, phrase in EVENT_VOICE_MAP.items():
                        print(f"  - {Colors.BOLD}{evt:<20}{Colors.RESET} : \"{phrase}\"")
                    print()
                elif verb == "test":
                    if len(parts) > 1:
                        target = parts[1].upper()
                        if target in EVENT_VOICE_MAP:
                            phrase = EVENT_VOICE_MAP[target]
                            print(f"{Colors.GREEN}[Test]{Colors.RESET} Speaking: \"{phrase}\"")
                            self.speak(phrase)
                        else:
                            print(f"{Colors.YELLOW}[Test Custom]{Colors.RESET} Speaking: \"{parts[1]}\"")
                            self.speak(parts[1])
                    else:
                        print(f"{Colors.GREEN}[Test]{Colors.RESET} Speaking key alerts...")
                        self.speak("Accurately human detected.")
                        self.speak("Access granted.")
                elif verb == "help":
                    print("Commands: status, reset, unlock, lock, test [event], events, quit")
                else:
                    self.send_command(cmd_line)
            except (KeyboardInterrupt, EOFError):
                self.running = False
                break

    def run(self):
        """Starts background reader and command shell."""
        connected = self.connect()
        if not connected:
            print(f"{Colors.YELLOW}[!] Starting in standalone voice test mode (no serial device). You can test voice commands.{Colors.RESET}")

        listener_thread = threading.Thread(target=self.serial_listener, daemon=True)
        listener_thread.start()

        try:
            self.interactive_shell()
        finally:
            self.running = False
            if self.serial_conn and self.serial_conn.is_open:
                try:
                    self.serial_conn.close()
                except Exception:
                    pass
            self.voice_worker.stop()
            print(f"{Colors.GREEN}[OK] Smart Locker Voice Monitor terminated cleanly.{Colors.RESET}")


# ==========================================
# 4. ENTRY POINT
# ==========================================
if __name__ == "__main__":
    specified_port = sys.argv[1] if len(sys.argv) > 1 else None
    app = SmartLockerMonitor(port=specified_port)
    app.run()
