# nPhoneKIT

**nPhoneKIT** is a fully open-source, community-powered toolbox for Android devices — built to replace bloated, sketchy, and closed-source tools that *hide* what they’re doing to your phone.

Unlike some other tools 👀 that are obfuscated, flagged by antivirus scanners, and hide everything behind cryptic buttons, **nPhoneKIT** is:

- ✅ 100% open Python code
- ✅ Actually shows you the commands
- ✅ Doesn’t ping weird servers or drop mystery EXEs in Temp folders
- ✅ Does most of the things that other tools can do anyway

### Why choose a tool that *won’t* tell you what it’s doing?
With **nPhoneKIT**, there’s no "magic click" – just real commands, real transparency, and zero smoke and mirrors.

---

### Bugs:
- We are currently aware of a bug that stops all actions from working when plugging in a phone. This will be fixed with the release of v3.0.0.

> [!IMPORTANT]
> If you find a bug, **make sure to open a GitHub issue.** This will help me fix the error and make nPhoneKIT better for everyone!

---

### Credits:
- **MediaTek features work only using MTKCLIENT, which is in the DEPS folder. Sourced from: https://github.com/bkerler/mtkclient. MTKCLIENT is provided by bkerler and IS NOT owned or created by nPhoneKIT in ANY WAY.**
- **IMEI Checking features work by opening a new tab of www.imei.info in order to check your IMEI yourself.**

## Installation

### Windows

- Go to the latest release, download Source Code as ZIP.
- Make sure you have Python and Pip installed.
- Extract the zip, open Command Prompt as Administrator, and cd into the source code directory.
- Run the following command:
  ```
  pip install pyserial requests
  ```
- Then, (every time you want to run nPhoneKIT you will need to run this in the source folder) (before running the below command, make sure Command Prompt is started as Administrator):
  python main.py

### Linux

- Go to the latest release, download Source Code as ZIP.
- Extract the zip, open Terminal, and cd into the source code directory.
- Run the following command:
  ```
  sudo apt install python3 python3-tk python3-serial python3-requests adb
  ```
- Then, (every time you want to run nPhoneKIT you will need to run this command in the source folder) (make sure to use SUDO):
  ```
  sudo python3 main.py
  ```

---

## 📱 Current Features

### Samsung
- FRP Unlock on most U.S. 2024 Samsung Devices 
- FRP Unlock on Aug 2022 - Dec 2022 Security Patch Date Samsung Devices
- FRP Unlock on Pre - Aug 2022 Security Patch Date Samsung Devices
- Get Version/Firmware Info on All Samsung Devices
- Reboot Samsung Devices (Normal or into Download Mode)
- Open Hidden WLANTEST Menu on All Samsung Devices
    
### LG
- Legacy Screen Unlock (Pre-G5)

### Generic Android
- Reboot

---

### 🔍 Transparency vs. Obfuscation

| Feature                  | **nPhoneKIT**         | SAMFW Tool                |
|--------------------------|-----------------------|---------------------------|
| Open source?             | ✅ Yes                | ❌ Not a chance (obfuscated with 5+ obfuscators) |
| Can you see commands?    | ✅ Yes                | ❌ Absolutely not         |
| VirusTotal results?      | ✅ Clean              | ❗ Flagged by 30+ AVs     |
| Works on Linux?          | ✅ Native Python code | ❌ Not really             |
| Entirely free, always?   | ✅ Absolutely         | ❌ Has a paid FRP unlock  |
| Works with other apps installed? | ✅ Totally!   | ❌ Literally checks for Wireshark and closes itself |

---

### Notes:

- nPhoneKIT has a feature called "Success Checks" which will contact external servers, telling them whether a said feature worked on your phone model. No personal data is sent. This helps improve nPhoneKIT.

---

### ⚖️ Legal

<sub>nPhoneKIT is a tool built entirely from original Python code. It does not include, link to, or distribute any copyrighted firmware, exploits, or proprietary binaries. Any similarity in function to other tools is the result of using standard public command sets (e.g. ADB, AT). This project is not affiliated with or endorsed by Samsung, LG, Google, Wireshark, or SamFW Tool. Trademarks used for descriptive purposes only.</sub>

---
