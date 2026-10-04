# 🔐 Intelligent Smart Locker & Anti-Theft Security System

Welcome to the **Intelligent Smart Locker** repository!

The complete project code, hardware pinouts, circuit diagrams, and documentation are organized inside the [`SmartLocker/`](file:///d:/Embedded%20system/SmartLocker) directory:

- 📄 **Arduino Firmware:** [`SmartLocker/Arduino/SmartLocker/SmartLocker.ino`](file:///d:/Embedded%20system/SmartLocker/Arduino/SmartLocker/SmartLocker.ino)
- 🎙️ **Python Voice & Security Monitor:** [`SmartLocker/Python/security_voice.py`](file:///d:/Embedded%20system/SmartLocker/Python/security_voice.py)
- 📦 **Python Dependencies:** [`SmartLocker/Python/requirements.txt`](file:///d:/Embedded%20system/SmartLocker/Python/requirements.txt)
- 📐 **System Architecture & Circuit Docs:** [`SmartLocker/docs/system_architecture.md`](file:///d:/Embedded%20system/SmartLocker/docs/system_architecture.md)
- 📖 **Full User Guide & Testing Guide:** [`SmartLocker/README.md`](file:///d:/Embedded%20system/SmartLocker/README.md)

### Quick Start:

1. **Upload Firmware:** Open [`SmartLocker/Arduino/SmartLocker/SmartLocker.ino`](file:///d:/Embedded%20system/SmartLocker/Arduino/SmartLocker/SmartLocker.ino) in Arduino IDE and upload to your Arduino UNO.
2. **Launch Voice Monitor:**
   ```powershell
   cd "d:\Embedded system\SmartLocker\Python"
   pip install -r requirements.txt
   python security_voice.py
   ```
3. Follow the [11-Phase Testing Guide](file:///d:/Embedded%20system/SmartLocker/README.md#5-step-by-step-11-phase-testing-guide) to verify hardware step-by-step.
