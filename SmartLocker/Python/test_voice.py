"""
Audio & Voice Synthesizer Diagnostic Tool
Tests Windows Text-to-Speech output directly through your laptop speakers.
"""
import sys

# Ensure UTF-8 output on Windows CMD to prevent encoding crashes
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def test_speech():
    print("========================================")
    print("  [*] Testing Laptop Voice Output...")
    print("========================================")

    # 1. Try Native Windows SAPI (Fastest and most reliable on Windows)
    try:
        import win32com.client
        print("[1/3] Testing Windows Native SAPI Voice...")
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Volume = 100
        speaker.Rate = 0
        speaker.Speak("Smart locker voice test. Accurately human detected.")
        print("[OK] Native Windows SAPI speech completed successfully!")
        return True
    except Exception as e:
        print(f"[!] Native SAPI failed: {e}")

    # 2. Try pyttsx3
    try:
        import pyttsx3
        print("[2/3] Testing pyttsx3 offline engine...")
        engine = pyttsx3.init()
        engine.setProperty("volume", 1.0)
        engine.setProperty("rate", 160)
        engine.say("Smart locker voice test with pyttsx3. Access granted.")
        engine.runAndWait()
        print("[OK] pyttsx3 speech completed successfully!")
        return True
    except Exception as e:
        print(f"[!] pyttsx3 failed: {e}")

    # 3. Fallback to Windows PowerShell SpeechSynthesizer
    try:
        import subprocess
        print("[3/3] Testing Windows PowerShell System.Speech...")
        cmd = 'Add-Type -AssemblyName System.Speech; [System.Speech.Synthesis.SpeechSynthesizer]::new().Speak("Emergency water alert. Danger Danger.")'
        subprocess.run(["powershell", "-NoProfile", "-Command", cmd], check=True)
        print("[OK] PowerShell speech completed successfully!")
        return True
    except Exception as e:
        print(f"[!] PowerShell speech failed: {e}")

    print("\n[X] All voice methods failed. Check if laptop speakers are muted or volume is turned down.")
    return False

if __name__ == "__main__":
    test_speech()
