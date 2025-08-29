# IMPORTS AND WHY EACH ONE IS NEEDED

import time # Waiting before executing something
import os # Executing most commands
import tkinter as tk # Main GUI
from tkinter import ttk # Styling for GUI (deprecated)
from tkinter import messagebox # Opening message/warning boxes
from tkinter import font # Customizing GUI font
from pathlib import Path # Importing deps (deprecated)
from serial.tools import list_ports # Listing connected devices
import sys # Getting basic system info
import re # Finding strings within text
import subprocess # Opening new processes
import serial # Communicating with device
import platform # Checking the current OS
import glob # Finding/listing ports
import asyncio # Running different actions asynchronously
import threading # Using multiple threads
import urllib.request # Requesting different servers
import json # Parsing and creating JSON
import requests # Requesting different servers
import uuid # Parsing and creating UUIDs
import hashlib # Hashing strings
import webbrowser # Opening browser to any page
import xml.etree.ElementTree as ET # Importing strings.xml
from PyQt5 import QtCore, QtGui, QtWidgets # GUI
from functools import partial

## nPhoneKIT permissions (these are the things that nPhoneKIT is capable of doing):

# Communicate with USB devices using ADB, MTP, and AT commands.
# Communicate with external servers to verify whether an action worked or not.
# Open a new tab in the default browser
# Checking and getting basic information about the current system

version = "1.3.3"

# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License as published by the Free Software Foundation, 
# either version 3 of the License, or any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
# See the LICENSE (included in the nPhoneKIT source) for more details.

# ===========================================================================================================

# Requirements:
#
# Ubuntu >=20.0.4-LTS
# Windows support exists but is not well-supported yet.
# At least 1 USB A or USB C port
# Python
# Everything in requirements.txt
# 
# ===================================
#
# Already-Installed Requirements:
#
# usbsend.py (by nPhoneKIT) # not needed anymore but still here
#
# ===================================


# CONFIG HAS BEEN MOVED TO SETTINGS.JSON, OR THE SETTINGS MENU IN THE nPhoneKIT GUI


# ============================================================================= #
# You shouldn't edit anything below this line unless you know what you're doing #
# ============================================================================= #

import json
from pathlib import Path

SETTINGS_PATH = Path("settings.json")

firstunlock = False

default_settings = {
    "dark_theme": False,
    "hacker_font": False,
    "slower_animations": False,
    "update_check": True,
    "impatient": False,
    "enable_preload": True,
    "debug_info": False,
    "i_know_what_im_doing": False,
    "basic_success_checks": True
}

# Load or initialize
if SETTINGS_PATH.exists():
    with open(SETTINGS_PATH, "r") as f:
        settings = json.load(f)
else:
    settings = default_settings.copy()
    with open(SETTINGS_PATH, "w") as f:
        json.dump(settings, f, indent=2)

# Inject into variables (your original style)
dark_theme = settings['dark_theme']
hacker_font = settings['hacker_font']
slower_animations = settings['slower_animations']
update_check = settings['update_check']
impatient = settings['impatient']
enable_preload = settings['enable_preload']
debug_info = settings['debug_info']
i_know_what_im_doing = settings['i_know_what_im_doing']
basic_success_checks = settings['basic_success_checks']

def load_strings(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()
    return {
        elem.attrib['name']: elem.text.replace('\\n', '\n') if elem.text else ''
        for elem in root.findall('string')
    }

# Load strings
strings = load_strings("strings.xml")

# Load settings
def load_settings():
    with open(SETTINGS_PATH, "r") as f:
        return json.load(f)

# Save settings
def save_settings(new_settings):
    with open(SETTINGS_PATH, "w") as f:
        json.dump(new_settings, f, indent=2)

# Settings GUI
def open_settings_window():
    settings = load_settings()
    win = tk.Toplevel()
    win.geometry('800x300')
    win.title(strings['settingsMenuTitleText'])

    vars = {}
    row = 0

    for key in ["dark_theme", "hacker_font", "slower_animations", "update_check",
                "impatient", "enable_preload"]:
        tk.Label(win, text=key).grid(row=row, column=0, sticky="w")
        var = tk.BooleanVar(value=settings[key])
        chk = tk.Checkbutton(win, variable=var)
        chk.grid(row=row, column=1)
        vars[key] = var
        row += 1

    def open_dev_settings():
        dev_win = tk.Toplevel()
        dev_win.title(strings['devSettingsTitle'])
        dev_vars = {}

        for i, key in enumerate(['debug_info", "i_know_what_im_doing", "basic_success_checks']):
            tk.Label(dev_win, text=key).grid(row=i, column=0, sticky="w")
            var = tk.BooleanVar(value=settings[key])
            chk = tk.Checkbutton(dev_win, variable=var)
            chk.grid(row=i, column=1)
            dev_vars[key] = var

        def apply_dev():
            for k, v in dev_vars.items():
                settings[k] = v.get()
            save_settings(settings)
            dev_win.destroy()

        tk.Button(dev_win, text=strings['applyText'], command=apply_dev).grid(row=i+1, columnspan=2)

    def apply_main():
        for k, v in vars.items():
            settings[k] = v.get()
        save_settings(settings)
        win.destroy()

    tk.Button(win, text=strings['applyText'], command=apply_main).grid(row=row, column=0)
    tk.Button(win, text=strings['devSettingsTitle'], command=open_dev_settings, font=("Segoe UI", 5), width=20, height=1).grid(row=row+1, column=0)

os_config = "WINDOWS" if platform.system() == "Windows" else "LINUX" # Auto-get OS and save to var

if os_config == "WINDOWS":
    enable_preload = False # Preload doesn't work on Windows; disable it

preload_done = threading.Event() # Event variable to check whether the Samsung modem preload has completed

ETA = [
    "not available", # Enabling ADB access
    "not available", # Running FRP unlock B
    "not available", # Running Screen Unlock
    "00:00:05", # Testing USB access
    "00:00:15", # Getting version info
    "00:00:10" # Opening WIFITEST
    "00:00:06" # Crashing a SAMSUNG phone to reboot
    "00:00:01" # Crashing a non-SAMSUNG phone to reboot (Seems to possibly work without modem unlock, on newer Samsungs, such as S24 series. Should look into this*)
]

class SerialManager: # AT command sender via class
    def __init__(self, baud=115200): # Start the serial port early
        self.port = self.detect_port() # Detect which port it is
        self.baud = baud # Choose a baud rate
        self.ser = None

        if not self.port: # No device connected
            if debug_info:
                print(strings['noDeviceSermanError']) 
        elif self.port:
            try:
                self.ser = serial.Serial(self.port, self.baud, timeout=2) # Save the port for use with the rest of the class
                time.sleep(0.5)
                if debug_info:
                    print(f"{strings['sermanConnectedPort']}{self.port}")
            except serial.SerialException as e:
                raise RuntimeError(f"{strings['sermanOpeningPortError']}{self.port}: {e}")

    def reset(self):
        self.__init__()

    def detect_port(self):
        system = platform.system()

        # Detect port for different systems/OSes

        if system == "Windows": 
            for i in range(1, 256):
                try:
                    s = serial.Serial(f"COM{i}")
                    s.close()
                    return f"COM{i}"
                except:
                    pass
        elif system == "Darwin":  # macOS
            ports = glob.glob("/dev/tty.usb*")
            return ports[0] if ports else None
        else:  # Linux
            ports = glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")
            return ports[0] if ports else None

        return None

    def send(self, command):
        if not self.ser or not self.ser.is_open:
            if preload_samsung_modem:
                if debug_info:
                    print(strings['noDeviceGenericError'])
            else:
                print(strings['noDeviceGenericError'])
        else:
            self.ser.flushInput()
            self.ser.flushOutput()
            self.ser.write((command.strip() + '\r\n').encode())
            time.sleep(0.1)

            output = []
            while True:
                line = self.ser.readline()
                if not line:
                    break
                output.append(line.decode(errors='ignore').strip())

            return '\n'.join(output)

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

class SerialManagerWindows: # Version of SerialManager class specifically for Windows
    def __init__(self, port: str = None, baud: int = 115200, debug: bool = False):
        """
        Windows-only serial helper.
        :param port: Override COM port (e.g. "COM3"). If None, auto-detects.
        :param baud: Baud rate.
        :param debug: Print connection details if True.
        """
        if platform.system() != "Windows":
            raise RuntimeError(strings['sermanWindowsOsError'])

        self.debug = debug
        self.baud = baud
        self.ser = None

        # allow override, else auto-detect
        self.port = port or self.detect_port()
        if not self.port:
            raise RuntimeError(strings['sermanNoComPort'])

        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2)
            time.sleep(0.5)
            if self.debug:
                print(f"{strings['sermanConnectedPort']}{self.port} @ {self.baud} baud")
        except serial.SerialException as e:
            raise RuntimeError(f"{strings['sermanOpeningPortError']}{self.port}: {e}")
        
    def reset(self):
        self.__init__()

    def detect_port(self) -> str:
        """Return the first COM* port or None."""
        ports = list_ports.comports()
        if self.debug:
            print(f"{strings['sermanWinAvailablePorts']}{[p.device for p in ports]}")
        for p in ports:
            if p.device.upper().startswith("COM"):
                if self.debug:
                    print(f"{strings['sermanWinDev']}{p.device}")
                return p.device
        return None

    def send(self, command: str, wait: float = 0.1) -> str:
        """
        Send a command and collect all response lines.
        :param command: Text/AT command to send.
        :param wait: Seconds to pause before reading.
        """
        if not self.ser or not self.ser.is_open:
            raise RuntimeError(strings['serPortNotOpen'])

        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        self.ser.write((command.strip() + "\r\n").encode())
        time.sleep(wait)

        lines = []
        while True:
            line = self.ser.readline()
            if not line:
                break
            lines.append(line.decode(errors="ignore").strip())
        return "\n".join(lines)

    def close(self):
        """Close the serial connection."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            if self.debug:
                print(strings['sermanWinConClosed'])

if os_config == "WINDOWS": # Choose which serial manager to use based on OS
    serman = SerialManagerWindows()
elif os_config == "LINUX":
    serman = SerialManager()

class AT:
    def send(command, not_first=False):
        # Making usbsend.py into a built-in class (SerialManager for Linux, or SerialManagerWindows for Windows) improves command speed by 10-20x, and improves multi-OS compatibility
        rt()
        if enable_preload:
            preload_done.wait()
        if not_first:
            serman.reset()
        with open("tmp_output.txt", "w", encoding="utf-8") as f:
            try:
                result = serman.send(command)
                if result is None:
                    result = serman.send(command)
                    if result is None: # (If result is STILL None)
                        result = "" # Then give up after the second try.
                f.write(result)
            except Exception: # If the connection isn't there, reset to attempt to gain the connection back
                serman.reset()
                time.sleep(1) 
                try:
                    result = serman.send(command)
                    if result is None:
                        result = serman.send(command)
                        if result is None: # (If result is STILL None)
                            result = "" # Then give up after the second try.
                    f.write(result)
                except Exception:
                    # Device must not be plugged in?
                    print(strings['deviceConCheckNotPlugged'])
    
class ADB: # ADB class for sending ADB commands if needed
    def send(command):
        rt()
        if os_config == "LINUX":
            os.system(f"sudo bash -c 'sudo adb {command} > tmp_output_adb.txt 2>&1'")
        elif os_config == "WINDOWS":
            with open('tmp_output.txt', 'w') as f:
                subprocess.run(['adb', command], stdout=f, stderr=subprocess.STDOUT)
        time.sleep(0.5)
    
    def usbswitch(arg, action):
        # Later, add logic to allow switching of device interface to AT, for more compatibility.
        return True
    
async def preload_samsung_modem(serman2):
    global enable_preload
    global preload_error

    if not enable_preload:
        preload_done.set() # If preload isn't enabled, pretend as if preload has already succeeded
        return

    try:
        system = platform.system()
        output = ""

        if system == "Linux": # Find connected devices on different OSes
            output = subprocess.check_output(['lsusb']).decode().lower()
        elif system == "Darwin":
            output = subprocess.check_output(['system_profiler', 'SPUSBDataType']).decode().lower()
        elif system == "Windows":
            output = subprocess.check_output(['powershell', 'Get-PnpDevice']).decode().lower()

        if "samsung" in output.lower():
            if debug_info:
                print(strings['samPreloadUsbDetected'])
            # If a Samsung device is plugged in, preload its modem using the below commands
            set_brand("Samsung") # For convenience, auto-select the SAMSUNG menu in the nPhoneKIT GUI
            serman2.send("AT+SWATD=0")  # Send without await since it's serial and blocking
            serman2.send("AT+ACTIVATE=0,0,0") # This and the above command do the same thing as modemUnlock("SAMSUNG"), except without infinitely waiting for preload_done, since modemUnlock uses the AT class which will follow preload_done
            if debug_info:
                print(strings['samPreloadComplete'])
            preload_error = False
        else:
            if debug_info:
                print(strings['samNoUsbFound'])
            enable_preload = False
            preload_error = True

    except Exception as e:
        if debug_info:
            print(strings['samPreloadError'], e) # Usually error, but works most of the time reguardless.
        enable_preload = False
        preload_error = True

    preload_done.set()


# Check for updates

def check_for_update():
    try:
        repo = "nlckysolutions/nPhoneKIT"
        url = f"https://api.github.com/repos/{repo}/releases/latest"

        with urllib.request.urlopen(url) as response:
            data = json.loads(response.read().decode())

            latest_version_raw = data['tag_name']
            latest_version = data['tag_name'].lstrip("v")

            # If the tag is different then the current version, assume it's newer, and prompt update.

            # Based on the unicode "v", depending on whether it's normal or U+2174, prompt for normal update and FORCE for critical update

            if latest_version != version:
                if "ⅴ" in latest_version_raw:
                    messagebox.showinfo(
                        strings['updateReqd'],
                        strings['updateReqdString'].format(version=version, latest_version=latest_version)
                    )
                    sys.exit(0) # Exit and do not let user use nPhoneKIT if the update is REQUIRED or critical
                else:   
                    messagebox.showinfo(
                        strings['updateAvail'],
                        strings['updateAvailString'].format(version=version, latest_version=latest_version)
                    )
    except Exception as e:
        print(strings['updateCheckFailed'])

def get_public_hardware_uuid():
    mac = uuid.getnode()
    mac_str = str(mac).encode('utf-8')

    # Hash the MAC so it's not identifying
    hashed_mac = hashlib.sha256(mac_str).hexdigest()

    # Optionally convert to UUID format (UUID5 with a fixed namespace)
    return uuid.UUID(hashlib.md5(hashed_mac.encode()).hexdigest())

FIREBASE_URL = "https://nphonekit-default-rtdb.firebaseio.com/" # URL for success checks

def success_checks(uuid, model, action, status, first=True):
        if basic_success_checks:
            if first:
                data = {
                    "timestamp": time.time(), # Basic success check info
                    "uuid": str(uuid), # Private hashed identifier in order to get anonymous active user estimation
                    "model": model.group(1) if model else "Unknown", # Check what model that the below action works on, anonymously
                    "action": action, # The action, for example "FRP_Unlock_2024"
                    "status": status, # Whether the action succeeded or failed
                    "phoneKITversion": version # Version of nPhoneKIT to get anonymous version usage estimation
                }

                try:
                    response = requests.post(f"{FIREBASE_URL}/success_checks.json", json=data)
                except Exception as e:
                    silentError = 1
            else:
                data = {
                    "timestamp": time.time(), # Same stuff as above, in order to get an anonymous active user estimation
                    "uuid": str(uuid),
                    "model": "NOT_First",
                    "action": "NOT_First",
                    "status": "Success",
                    "phoneKITversion": version
                }

                try:
                    response = requests.post(f"{FIREBASE_URL}/success_checks.json", json=data)
                except Exception as e:
                    silentError = 1

# =============================================
#  Different instructions for the user
# =============================================

def MTPmenu():
    show_messagebox_at(500, 200, "nPhoneKIT", strings['mtpMenu'])
    # Show user instructions to enable MTP mode

def adbMenu():
    show_messagebox_at(500, 200, "nPhoneKIT", strings['adbMenu'])
    # Show user instructions to enable ADB mode

# ================================================
#  Simple functions to eliminate repetitive tasks
# ================================================

def rt(): # Flush the output buffer. May be deprecated and replaced soon with a new output collection method
    if os_config == "LINUX": # Flush output buffer on different OSes
        os.system("sudo bash -c 'rm -f tmp_output.txt'") 
        os.system("sudo bash -c 'rm -f tmp_output_adb.txt'")
    elif os_config == "WINDOWS":
        os.system("del /F tmp_output.txt")
        os.system("del /F tmp_output_adb.txt")

def readOutput(type): # Read the output buffer based on command type AT or ADB
    if type == "AT":
        with open("tmp_output.txt", "r") as f:
            output = f.read()
    elif type == "ADB":
        with open("tmp_output_adb.txt", "r") as f:
            output = f.read()
    return output

def show_messagebox_at(x, y, title, content): # Show a customizable message box
    # Create a new top-level window
    box = tk.Tk() 
    box.title(title)
    box.geometry(f"+{x}+{y}")
    box.resizable(False, False)

    # Frame and Label
    tk.Label(box, text=content, font=("Segoe UI", 12), padx=20, pady=20).pack()

    # OK button that closes the window
    tk.Button(box, text="OK", width=10, command=box.destroy).pack(pady=(0, 15))

    # Keep it modal — BLOCK everything until this window closes
    box.attributes("-topmost", True)
    box.grab_set()
    box.wait_window()  # <--- THIS is what blocks until closed

def modemUnlock(manufacturer, softUnlock=False): # Unlock the modem per-action if preload wasn't enabled
    global firstunlock

    if os_config == "LINUX":
        if not enable_preload:
            if preload_error and firstunlock == False:
                if manufacturer == "SAMSUNG": # Select the manufacturer to preload
                    AT.send("AT+SWATD=0", True) # Disables some sort of a proprietary "AT commands lock" from SAMSUNG
                    AT.send("AT+ACTIVATE=0,0,0", True) # An activation sequence that unlocks the modem when paired with the above command.
                    firstunlock = True
            else:
                if manufacturer == "SAMSUNG": # Select the manufacturer to preload
                    if softUnlock:
                        AT.send("AT+SWATD=0") # Disables some sort of a proprietary "AT commands lock" from SAMSUNG
                    else:
                        AT.send("AT+SWATD=0") # Disables some sort of a proprietary "AT commands lock" from SAMSUNG
                        AT.send("AT+ACTIVATE=0,0,0") # An activation sequence that unlocks the modem when paired with the above command.
    elif os_config == "WINDOWS":
        if manufacturer == "SAMSUNG": # Select the manufacturer to preload
            if softUnlock:
                AT.send("AT+SWATD=0") # Disables some sort of a proprietary "AT commands lock" from SAMSUNG
            else:
                AT.send("AT+SWATD=0") # Disables some sort of a proprietary "AT commands lock" from SAMSUNG
                AT.send("AT+ACTIVATE=0,0,0") # An activation sequence that unlocks the modem when paired with the above command.

# Function that can parse DEVCONINFO in order to make it more readable
def parse_devconinfo(raw_input): 
    lines = raw_input.strip().splitlines()
    parsed_output = []

    for line in lines:
        if "+DEVCONINFO:" in line:
            # Extract the part after "+DEVCONINFO:"
            content = line.split(":", 1)[1].strip()
            # Split by semicolon
            items = content.split(";")
            for item in items:
                if not item:
                    continue
                match = re.match(r'(\w+)\((.*?)\)', item)
                if match:
                    key, value = match.groups()
                    friendly_key = {
                        "MN": "Model",
                        "BASE": "Baseband",
                        "VER": "Software Version",
                        "HIDVER": "Hidden Version",
                        "MNC": "Mobile Network Code",
                        "MCC": "Mobile Country Code",
                        "PRD": "Product Code",
                        "AID": "App ID",
                        "CC": "Country Code",
                        "OMCCODE": "OMC Code",
                        "SN": "Serial Number",
                        "IMEI": "IMEI",
                        "UN": "Unique Number",
                        "PN": "Phone Number",
                        "CON": "Connection Types",
                        "LOCK": "SIM Lock",
                        "LIMIT": "Limit Status",
                        "SDP": "SDP Mode",
                        "HVID": "Partition Info"
                    }.get(key, key)
                    parsed_output.append(f"{friendly_key}: {value if value else 'N/A'}")
    return "\n".join(parsed_output)

def testAT(MTPinstruction=False, text=f"Testing USB access (ETA: {ETA[3]})..."): # Deprecated
    return True

# =============================================
#  Unlocking methods for different devices
# =============================================

def frp_unlock_pre_aug2022(): # FRP unlock for pre-aug2022 security patch update
    print(strings['getVerInfo'], end="")
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output

    if info == "Fail":
        print(strings['deviceCheckPluggedIn2'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Pre_2022", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
    else:
        ATcommands = [
            "AT+DUMPCTRL=1,0",
            "AT+DEBUGLVC=0,5",
            "AT+SWATD=0", # Removes some kind of proprietary SAMSUNG modem lock
            "AT+ACTIVATE=0,0,0", # So that you can ACTIVATE
            "AT+SWATD=1", # Then relocks it.
            "AT+DEBUGLVC=0,5"
        ]

        ADBcommands = [ # Run list of commands in order to complete the unlock with newly-enabled ADB
            "shell settings put global setup_wizard_has_run 1",
            "shell settings put secure user_setup_complete 1",
            "shell content insert --uri content://settings/secure --bind name:s:DEVICE_PROVISIONED --bind value:i:1",
            "shell content insert --uri content://settings/secure --bind name:s:user_setup_complete --bind value:i:1",
            "shell content insert --uri content://settings/secure --bind name:s:INSTALL_NON_MARKET_APPS --bind value:i:1",
            "shell am start -c android.intent.category.HOME -a android.intent.action.MAIN"
        ]

        show_messagebox_at(500, 200, "nPhoneKIT", strings['misuseFrpGuidance'])

        print(strings['attemptingEnableAdb'], end="")

        show_messagebox_at(500, 200, "nPhoneKIT", strings['frpUnlockStepsPre2022'])

        for command in ATcommands:
            AT.send(command)

        output = readOutput("AT")

        if "error" in output.lower():
            print(strings['failText'])
            print(strings['frpNotCompatible'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Pre_2022", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            print(strings['okText'])
            print(strings['runUnlock'], end="")
            show_messagebox_at(500, 200, "nPhoneKIT", strings['usbDebuggingPromptCheck'])
            for command in ADBcommands:
                ADB.send(command)
            print(strings['okText'])
            print(strings['unlockSuccess'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Pre_2022", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

def frp_unlock_aug2022_to_dec2022(): # FRP unlock for aug2022-dec2022 security patch update
    print(strings['getVerInfo'], end="")
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output

    if info == "Fail":
        print(strings['deviceCheckPluggedIn2'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Aug_To_Dec_2022", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
    else:
        commands = ['AT+SWATD=0', 'AT+ACTIVATE=0,0,0', 'AT+DEVCONINFO','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0', 'AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5']
        # These commands are supposed to overwhelm the phone and trick it into enabling ADB. The rest after this is the same as the other unlock method.

        ADBcommands = [ # Run list of commands in order to complete the unlock with newly-enabled ADB
            "shell settings put global setup_wizard_has_run 1",
            "shell settings put secure user_setup_complete 1",
            "shell content insert --uri content://settings/secure --bind name:s:DEVICE_PROVISIONED --bind value:i:1",
            "shell content insert --uri content://settings/secure --bind name:s:user_setup_complete --bind value:i:1",
            "shell content insert --uri content://settings/secure --bind name:s:INSTALL_NON_MARKET_APPS --bind value:i:1",
            "shell am start -c android.intent.category.HOME -a android.intent.action.MAIN"
        ]

        show_messagebox_at(500, 200, "nPhoneKIT", strings['misuseFrpGuidance2022'])

        print(strings['attemptingEnableAdb'], end="")

        show_messagebox_at(500, 200, "nPhoneKIT", strings['frpUnlockSteps2022'])

        for command in commands:
            AT.send(command)

        output = readOutput("AT")

        if "error" in output.lower():
            print(strings['failText'])
            print(strings['frpNotCompatible'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Aug_To_Dec_2022", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            print(strings['okText'])
            print(strings['runUnlock'], end="")
            show_messagebox_at(500, 200, "nPhoneKIT", strings['usbDebuggingPromptCheck'])
            for command in ADBcommands:
                ADB.send(command)
            print(strings['okText'])
            print(strings['unlockSuccess'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Aug_To_Dec_2022", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

def frp_unlock_2024(): # FRP unlock for early 2024-ish security patch update
    print(strings['getVerInfo'], end="")
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output

    if info == "Fail":
        print(strings['deviceCheckPluggedIn2'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_2024", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
    else:
        commands = [
            "AT+SWATD=0", # Modem unlocking
            "AT+ACTIVATE=0,0,0", # Modem unlocking
            "AT+DEVCONINFO", # Get device info
            "AT+VERSNAME=3.2.3", # FRP unlocking commands
            "AT+REACTIVE=1,0,0", # FRP unlocking commands
            "AT+SWATD=0", # Re-Modem unlocking
            "AT+ACTIVATE=0,0,0", # Re-Modem unlocking
            "AT+SWATD=1", # Lock quickly
            "AT+SWATD=1", # Lock again
            "AT+PRECONFIG=2,VZW", # Quickly change CSC
            "AT+PRECONFIG=1,0", # Quickly change it back
        ]

        ADBcommands = [ # Run list of commands in order to complete the unlock with newly-enabled ADB
            "shell settings put global setup_wizard_has_run 1", 
            "shell settings put secure user_setup_complete 1",
            "shell content insert --uri content://settings/secure --bind name:s:DEVICE_PROVISIONED --bind value:i:1",
            "shell content insert --uri content://settings/secure --bind name:s:user_setup_complete --bind value:i:1",
            "shell content insert --uri content://settings/secure --bind name:s:INSTALL_NON_MARKET_APPS --bind value:i:1",
            "shell am start -c android.intent.category.HOME -a android.intent.action.MAIN"
        ]

        show_messagebox_at(500, 200, "nPhoneKIT", strings['misuseFrpGuidance2024'])

        print(strings['attemptingEnableAdb'], end="")

        show_messagebox_at(500, 200, "nPhoneKIT", strings['frpUnlockSteps2024'])

        for command in commands:
            AT.send(command)

        output = readOutput("AT")

        if "error" in output.lower():
            print(strings['failText'])
            print(strings['frpNotCompatible'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_2024", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            print(strings['okText'])
            print(strings['runUnlock'], end="")
            show_messagebox_at(500, 200, "nPhoneKIT", strings['usbDebuggingPromptCheck'])
            for command in ADBcommands:
                ADB.send(command)
            print(strings['okText'])
            print(strings['unlockSuccess'])
            if model == "" or model == None:
                # Retry get model
                info = verinfo(False, False)
                model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_2024", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

def general_frp_unlock(): # Not completed yet
    info = verinfo(False)
    if "Model: SM" in info:
        frp_unlock_pre_aug2022()
    else:
        # to do, add FULLY universal FRP unlock
        print(strings['deviceNotSupportedUniversal'])

def LG_screen_unlock(): # Screen unlock on supported LG devices *untested*
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output (may not work)

    show_messagebox_at(500, 200, "nPhoneKIT", strings['lgScreenUnlockSupportedDevs'])
    print(strings['lgRunningScreenUnlock'], end="")
    # Prepare phone for unlock
    show_messagebox_at(600, 100, "nPhoneKIT", strings['lgScreenUnlockSteps'])
    
    time.sleep(1)
    if AT.usbswitch("-l", "LG Screen Unlock"):
        rt() # Flush the output buffer
        AT.send('AT%KEYLOCK=0') # This AT command SHOULD unlock the screen instantly. (yes, one command.)
        with open("tmp_output.txt", "r") as f:
            output = f.read()
        # debug only: print("\n\nOutput: \n\n" + output + "\n\n")
        if "error" in output or "Error" in output:
            print(strings['failText'] + "\n")
            print(strings['lgScreenUnlockError'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "LG_Screen_Unlock", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            rt()
            print(strings['okText'] + "\n")
            print(strings['lgScreenUnlockSuccess'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "LG_Screen_Unlock", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

# ==============================================
#  Simple functions that do stuff to the device
# ==============================================

def verinfo(gui=True, showtext=True): # Get version info on the device. Pretty simple. (not simple, this has taken me hours.)
    if gui:
        if enable_preload: # Skip all the nonsense and cut straight to the action, no "testAT" nonsense. We're prioritizing speed.
            print(strings['getVerInfo'], end="")
            AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
            output = readOutput("AT") # Output is retrieved from the command
            if output == "" or output == None:
                AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
                output = readOutput("AT")
                if output == "" or output == None:
                    print(strings['failText'])
                    print(strings['verInfoCheckConn'])
                    model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                    tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Fail"))
                    tthread.start() # Sends basic, anonymized success_checks info with only the model number.
            else:
                print(strings['okText'])
                model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                tthread.start() # Sends basic, anonymized success_checks info with only the model number.
            output = parse_devconinfo(output) # Make the output actually readable
            print(output) # Print the version info to the output box
        else: 
            print(strings['getVerInfo'], end="")
            if testAT(True, text=strings['getVerInfo']): # We should verify AT is working before running the below code (testAT is deprecated)
                if not enable_preload:
                    modemUnlock("SAMSUNG") # Run the command to allow more AT access for SAMSUNG devices unless preloading is enabled
                    rt() # Flush the command output file
                AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
                output = readOutput("AT") # Output is retrieved from the command
                if output == "" or output == None:
                    AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
                    output = readOutput("AT")
                    if output == "" or output == None:
                        AT.send("AT+DEVCONINFO", True) # Only works when the modem is working with modemUnlock("SAMSUNG")
                        output = readOutput("AT")
                        try:
                            if output == "" or output == None:
                                print(strings['failText'])
                                print(strings['verInfoCheckConn'])
                            else:
                                output = parse_devconinfo(output) # Make the output actually readable
                                model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                                tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                                tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                                print(strings['okText'])
                        except Exception:
                            print(strings['verInfoCheckConn'])
                    else:
                        output = parse_devconinfo(output) # Make the output actually readable
                        model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                        tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                        print(strings['okText'])
                        print(output) # Print the version info to the output box
                else:
                    output = parse_devconinfo(output) # Make the output actually readable
                    model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                    tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                    tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                    print(strings['okText'])
                    print(output) # Print the version info to the output box
    else:
        #print(strings['getVerInfo'], end="")
        if testAT(True, text=strings['getVerInfo']): # We should verify AT is working before running the below code (deprecated)
            if not enable_preload:
                modemUnlock("SAMSUNG") # Run the command to allow more AT access for SAMSUNG devices unless preloading is enabled
                rt() # Flush the command output file
            AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
            output = readOutput("AT") # Output is retrieved from the command
            if output == "" or output == None:
                AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
                output = readOutput("AT")
                if output == "" or output == None:
                    if showtext:
                        print(strings['failText'])
                else:
                    if showtext:
                        print(strings['okText'])
            output = parse_devconinfo(output) # Make the output actually readable (parse the output)
            model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
            if output == "" or output == None:
                tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Fail"))
                tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                return "Fail"
            else:
                tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                return output # Return the version info

def wifitest(): # Opens a hidden WLANTEST menu on Samsung devices
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info)
    success = [
    "AT+WIFITEST=9,9,9,1",
    "+WIFITEST:9,",
    "OK"
    ]

    print(strings['openingWifitest'], end="")
    MTPmenu()
    modemUnlock("SAMSUNG") # Unlock modem
    AT.send("AT+SWATD=1") # Modem must be relocked for this to work
    rt()
    AT.send("AT+WIFITEST=9,9,9,1") # WifiTEST command to open
    output = readOutput("AT")
    counter = 0
    for i in success:
        if i in output:
            counter += 1
    if counter == 3:
        print(strings['okText'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "WIFITEST", "Success"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    else:
        print(strings['failText'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "WIFITEST", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def reboot(): # Crash an android phone to reboot
    print(strings['crashingToReboot'], end="")
    MTPmenu()
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info)
    rt()
    try:
        AT.send("AT+CFUN=1,1") # Crashes the phone immediately.
    except Exception as e:
        if "disconnected" in str(e):
            print(strings['okText']) # Error opening serial means that the command worked, because it reset the phone before it could give a response.
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    output = readOutput("AT")
    if "OK" in output:
        print(strings['failText'])
        print(strings['crashRebootFailed'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def reboot_sam(): # Crash a Samsung phone to reboot
    print(strings['crashingToReboot'], end="")
    MTPmenu()
    modemUnlock("SAMSUNG", True)
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info)
    rt()
    try:
        AT.send("AT+CFUN=1,1") # Crashes the phone immediately.
    except Exception as e:
        if "disconnected" in str(e):
            print(strings['okText']) # Error opening serial means that the command worked, because it reset the phone before it could give a response.
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT_SAM", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    output = readOutput("AT")
    if "OK" in output:
        print(strings['failText'])
        print(strings['crashRebootFailed'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT_SAM", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def bloatRemove():
    print(strings['uninstallingPackages'], end="")
    adbMenu()
    # Samsung ONLY
    packages = [
        # Samsung default bloatware
        "com.microsoft.office.outlook",
        "com.samsung.android.bixby.ondevice.frfr",
        "com.google.android.apps.photos",
        "com.sec.android.app.sbrowser",
        "com.samsung.android.calendar",
        "com.samsung.android.app.reminder",
        "com.google.android.apps.youtube.music",
        "com.sec.android.app.shealth",
        "com.samsung.android.nmt.apps.t2t.languagepack.enfr",
        "com.sec.android.app.popupcalculator",
        "com.booking.aidprovider",
        "com.samsung.SMT.lang_en_us_l03",
        "com.samsung.android.bixby.ondevice.enus",
        "com.google.android.apps.docs",
        "com.samsung.android.arzone",
        "com.samsung.android.voc",
        "com.samsung.android.app.tips",
        "com.sec.android.app.clockpackage",
        "com.samsung.android.app.find",
        "com.samsung.android.app.notes",
        "com.amazon.appmanager",
        "com.google.android.videos",
        "com.sec.android.app.voicenote",
        "com.amazon.mShop.android.shopping",
        "com.facebook.katana",
        "com.samsung.sree",
        "com.samsung.android.app.spage",
        "com.samsung.android.oneconnect",
        "com.samsung.android.game.gamehome",
        "com.samsung.SMT.lang_fr_fr_l01",
        "com.microsoft.office.officehubrow",
        "com.samsung.android.spay",
        "com.samsung.android.app.watchmanager",
        "com.samsung.android.tvplus",
        "com.sec.android.app.kidshome",
        "com.booking",
        # Verizon bloatware
        "com.verizon.appmanager",
        "com.vzwnavigator",
        "com.vzw.syncservice",
        "com.verizon.syncservice",
        "com.verizon.login",
        "com.vzw.voicemail",
        "com.vzw.nflmobile",
        "com.vzw.familybase",
        "com.vzw.familylocator",
        # AT&T bloatware
        "com.att.devicehelp",
        "com.att.addressbooksync",
        "com.dti.att",
        "com.dti.folderlauncher",
        "com.myatt.mobile",
        # T-Mobile bloatware
        "com.tmobile.nameid",
        "com.tmobile.visualvm",
        "com.tmobile.account",
        "com.tmobile.appmanager",
        "com.tmobile.appselector",
        "com.tmobile.pr.mytmobile",
        "com.tmobile.echolocate",
        "com.ironsrc.aura.tmo",
        "com.tmobile.pr.adapt"
    ]
    for package in packages:
        ADB.send(f"shell pm uninstall --user 0 {package}")
        if "Success" in readOutput("ADB") or "[n" in readOutput("ADB") or "age:" in readOutput("ADB"):
            continue
        else:
            print(strings['failText'])
            print(strings['devNotConnectedOrOtherErr'])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), None, "DEBLOAT_SAM", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number.
            break
    if "Success" in readOutput("ADB") or "[n" in readOutput("ADB") or "age:" in readOutput("ADB"):
        print(strings['okText'])
        print(strings['debloatSucceeded'])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), None, "DEBLOAT_SAM", "Success"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def reboot_download_sam(): # Reboot Samsung device to download mode
    print(strings['rebootingDownloadMode'], end="")
    MTPmenu() 
    AT.send("AT+FUS?") # Thankfully, no modem unlocking required for this command.
    if basic_success_checks:
        modemUnlock("SAMSUNG")
        info = verinfo(False)
        model = re.search(r'Model:\s*(\S+)', info)
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT_DOWNLOAD_SAM", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    print(" OK")

def imeicheck():
    info = verinfo(False)
    match = re.search(r'IMEI:\s*([0-9]+)', info)
    if match:
        imei = match.group(1)
        messagebox.showinfo("nPhoneKIT", strings['imeiCheckGuide'])
        if os_config == "WINDOWS":
            webbrowser.open_new_tab(f"https://www.imei.info/services/blacklist-simple/samsung/check-free/?imei={str(imei)}")
        elif os_config == "LINUX":
            url = f"https://www.imei.info/services/blacklist-simple/samsung/check-free/?imei={str(imei)}"
            original_user = os.environ.get("SUDO_USER", "yourusername")  # linux is complicated :/
            cmd = f'su - {original_user} -c "DISPLAY=$DISPLAY DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS xdg-open \\"{url}\\""'
            os.system(cmd)
        print(strings['imeiChecked'])
    else:
        print(strings['imeiNotFound'])

def mtkclient():
    if os_config == "WINDOWS":
        os.system('pip install -r deps/mtkclient/requirements.txt')
        os.system('python ./deps/mtkclient/mtk_gui.py')
    elif os_config == "LINUX":
        #os.system('sudo pip install --no-deps statsd scrypt repoze.lru keystone-engine fusepy aniso8601 Yappi wrapt werkzeug WebOb vine unicorn tzdata testtools shiboken6 Routes rfc3986 pyusb pyflakes pycryptodomex pycryptodome pycodestyle psutil prometheus-client PrettyTable pbr PasteDeploy Paste netaddr msgpack mccabe itsdangerous iso8601 greenlet elementpath dnspython capstone cachetools blinker xmlschema testscenarios testresources stevedore SQLAlchemy PySide6-Essentials oslo.i18n oslo.context os-service-types Flask flake8 eventlet debtcollector amqp PySide6-Addons pysaml2 oslo.utils oslo.config kombu keystoneauth1 futurist Flask-RESTful dogpile.cache alembic pyside6 oslo.serialization oslo.middleware oslo.db oslo.concurrency python-keystoneclient pycadf osprofiler oslo.policy oslo.log oslo.upgradecheck oslo.service oslo.metrics oslo.cache oslo.messaging keystonemiddleware keystone --break-system-packages')
        #os.system('sudo python3 deps/mtkclient/mtk_gui.py')
        os.system('sudo apt install libxcb-cursor0')
        os.system("sudo bash -c 'source ./deps/venv/bin/activate && python3 ./deps/mtkclient/mtk_gui.py'")

def featureRequest():
    # to do

    data = {
        "timestamp": time.time(), 
        "uuid": str(uuid),
        "featureDesc": featureDesc,
        "phoneKITversion": version
    }

    try:
        response = requests.post(f"{FIREBASE_URL}/success_checks.json", json=data)
        status = "Feature request submitted successfully!"
    except Exception as e:
        status = "Error: Feature request failed to send. Check your connection?"
    return status

# ===================================
#  PyQt5 GUI Stuff
# ===================================

# ------------ theme & assets helpers ------------
ACCENT = "#7C4DFF"        # deep purple accent (material-ish)
ACCENT_HOVER = "#5E35B1"
SURFACE = "#121212"
SURFACE_2 = "#1E1E1E"
TEXT = "#EAEAEA"
TEXT_DIM = "#B9B9B9"
OK_COLOR = "#35D07F"
FAIL_COLOR = "#FF6B6B"

def _find_logo():
    for p in ("assets/logo.png", "logo.png", "./assets/logo.png", "./logo.png"):
        if os.path.exists(p):
            return p
    return None

def _material_qss(dark=True, hacker=False):
    base_font = "JetBrains Mono" if hacker else "Inter, 'Segoe UI', Roboto, Helvetica, Arial"
    mono_font = "JetBrains Mono" if hacker else "Fira Code, Consolas, 'Courier New'"
    if dark:
        return f"""
        * {{
            font-family: {base_font};
            color: {TEXT};
        }}
        QMainWindow {{
            background: {SURFACE};
        }}
        QToolTip {{
            background: #FFD54F;            /* warm yellow */
            color: #111111;                 /* readable text */
            border: 1px solid #E6B800;      /* subtle darker yellow border */
            padding: 6px 10px;
            border-radius: 8px;
            font-weight: 600;
            font-family: "Noto Color Emoji", Inter, 'Segoe UI', Roboto, Helvetica, Arial;
        }}
        QPushButton {{
            background: {SURFACE_2};
            border: 1px solid #2A2A2A;
            padding: 10px 14px;
            border-radius: 10px;
        }}
        QPushButton:hover {{ background: #262626; border-color: #333; }}
        QPushButton:pressed {{ background: {ACCENT}; color: white; }}
        QTabWidget::pane {{
            border: 1px solid #2A2A2A; border-radius: 12px; background: {SURFACE_2};
        }}
        QTabBar::tab {{
            background: transparent; color: {TEXT_DIM};
            padding: 10px 18px; margin: 6px; border-radius: 10px;
        }}
        QTabBar::tab:hover {{ background: #262626; color: {TEXT}; }}
        QTabBar::tab:selected {{ background: {ACCENT}; color: white; }}
        QFrame#Header {{
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 {ACCENT}, stop:1 {ACCENT_HOVER});
            border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;
        }}
        QLabel#AppTitle {{ color: white; font-size: 22px; font-weight: 700; }}
        QPlainTextEdit, QTextEdit {{
            background: #0E0E0E; border: 1px solid #2A2A2A; border-radius: 12px; padding: 10px;
            font-family: {mono_font}; font-size: 13px;
        }}
        QProgressBar {{
            border: 1px solid #2A2A2A; border-radius: 8px;
            background: #0E0E0E; text-align: center; color: {TEXT_DIM};
        }}
        QProgressBar::chunk {{ background: {ACCENT}; border-radius: 8px; }}
        QCheckBox::indicator {{
            width: 36px; height: 20px; border-radius: 10px; background: #2A2A2A;
        }}
        QCheckBox::indicator:checked {{ background: {ACCENT}; }}
        QCheckBox::indicator::handle {{
            width: 16px; height: 16px; margin: 2px; border-radius: 8px; background: #B0B0B0;
        }}
        QSplitter::handle {{ background: #1A1A1A; width: 6px; }}
        QPushButton {{
            font-family: "Noto Color Emoji";
        }}
        """
    else:
        # light mode (kept simple)
        return f"""
        * {{ color: #1A1A1A; font-family: {base_font}; }}
        QMainWindow {{ background: #FAFAFA; }}
        QToolTip {{
            background: #FFEB3B;            /* bright yellow for light theme */
            color: #111111;
            border: 1px solid #E6B800;
            padding: 6px 10px;
            border-radius: 8px;
            font-weight: 600;
            font-family: "Noto Color Emoji", Inter, 'Segoe UI', Roboto, Helvetica, Arial;
        }}
        QPushButton {{
            background: #FFFFFF; border: 1px solid #E0E0E0; padding: 10px 14px; border-radius: 10px;
        }}
        QPushButton:hover {{ background: #F2F2F2; }}
        QPushButton:pressed {{ background: {ACCENT}; color: white; }}
        QTabWidget::pane {{ border: 1px solid #E0E0E0; border-radius: 12px; background: #FFFFFF; }}
        QTabBar::tab {{
            background: transparent; color: #666;
            padding: 10px 18px; margin: 6px; border-radius: 10px;
        }}
        QTabBar::tab:hover {{ background: #F2F2F2; color: #222; }}
        QTabBar::tab:selected {{ background: {ACCENT}; color: white; }}
        QFrame#Header {{
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 {ACCENT}, stop:1 {ACCENT_HOVER});
        }}
        QLabel#AppTitle {{ color: white; font-size: 22px; font-weight: 700; }}
        QPlainTextEdit, QTextEdit {{
            background: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 12px; padding: 10px;
            font-family: {mono_font}; font-size: 13px; color: #222;
        }}
        QProgressBar {{
            border: 1px solid #E0E0E0; border-radius: 8px; background: #FFF; text-align: center; color: #555;
        }}
        QProgressBar::chunk {{ background: {ACCENT}; border-radius: 8px; }}
        QCheckBox::indicator {{
            width: 36px; height: 20px; border-radius: 10px; background: #DDD;
        }}
        QCheckBox::indicator:checked {{ background: {ACCENT}; }}
        QCheckBox::indicator::handle {{
            width: 16px; height: 16px; margin: 2px; border-radius: 8px; background: #FFF;
        }}
        QSplitter::handle {{ background: #EEE; width: 6px; }}
        QPushButton {{
            font-family: "Noto Color Emoji";
        }}
        """

# ------------ busy spinner overlay ------------
class BusyOverlay(QtWidgets.QWidget):
    def __init__(self, parent=None, text="Working…"):
        super().__init__(parent)
        self.setAttribute(QtCore.Qt.WA_TransparentForMouseEvents, False)
        self.setWindowFlags(QtCore.Qt.FramelessWindowHint | QtCore.Qt.SubWindow)
        self._angle = 0
        self._timer = QtCore.QTimer(self)
        self._timer.timeout.connect(self._tick)
        self._label = QtWidgets.QLabel(text, self)
        self._label.setStyleSheet(f"color:{TEXT}; font-size:14px;")
        self.hide()

    def _tick(self):
        self._angle = (self._angle + 8) % 360
        self.update()

    def start(self):
        self.setGeometry(self.parent().rect())
        self.show()
        self.raise_()
        self._timer.start(16)

    def stop(self):
        self._timer.stop()
        self.hide()

    def resizeEvent(self, e):
        self.setGeometry(self.parent().rect())
        self._label.adjustSize()
        self._label.move(self.width()//2 - self._label.width()//2, self.height()//2 + 26)
        super().resizeEvent(e)

    def paintEvent(self, e):
        p = QtGui.QPainter(self)
        p.setRenderHint(QtGui.QPainter.Antialiasing)
        # dim background
        p.fillRect(self.rect(), QtGui.QColor(0,0,0,120))
        # spinner donut
        radius = 22
        center = QtCore.QPoint(self.width()//2, self.height()//2 - 8)
        pen = QtGui.QPen(QtGui.QColor(255,255,255,220), 3)
        p.setPen(pen)
        # draw faint ring
        p.setOpacity(0.2); p.drawEllipse(center, radius, radius)
        # draw rotating arc
        p.setOpacity(1.0)
        p.save()
        p.translate(center)
        p.rotate(self._angle)
        rect = QtCore.QRectF(-radius, -radius, radius*2, radius*2)
        p.drawArc(rect, 0, 110*16)  # 110 degrees
        p.restore()

# ------------ worker to run blocking functions off UI thread ------------
class WorkerSignals(QtCore.QObject):
    finished = QtCore.pyqtSignal()
    error = QtCore.pyqtSignal(str)

class Worker(QtCore.QRunnable):
    def __init__(self, fn, *args, **kwargs):
        super().__init__()
        self.fn = fn
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()
        self.setAutoDelete(True)

    @QtCore.pyqtSlot()
    def run(self):
        try:
            self.fn(*self.args, **self.kwargs)
        except Exception as e:
            self.signals.error.emit(str(e))
        finally:
            self.signals.finished.emit()

# ------------ stdout redirector -> QTextEdit with token coloring ------------
class QtRedirectText(QtCore.QObject):
    new_text = QtCore.pyqtSignal(str)
    def __init__(self, widget):
        super().__init__()
        self.widget = widget
        self.pattern = re.compile(r"( FAIL| OK)")
        self.new_text.connect(self._append)

    def write(self, s: str):
        # ensure on gui thread
        self.new_text.emit(s)

    def flush(self):  # required for file-like
        pass

    def _append(self, s: str):
        # token colorize -> HTML
        def _esc(x): return QtGui.QTextDocument().toPlainText() if False else x  # no-op fast path
        parts = []
        last = 0
        for m in self.pattern.finditer(s):
            parts.append(QtGui.QTextDocument().toPlainText() if False else s[last:m.start()])
            token = m.group(1).strip()
            color = OK_COLOR if token == "OK" else FAIL_COLOR
            parts.append(f'<span style="color:{color}; font-weight:700;"> {token}</span>')
            last = m.end()
        parts.append(s[last:])
        html = "".join(parts).replace("\n", "<br>")
        self.widget.moveCursor(QtGui.QTextCursor.End)
        self.widget.insertHtml(html)
        self.widget.moveCursor(QtGui.QTextCursor.End)

# ------------ settings dialog ------------
class SettingsDialog(QtWidgets.QDialog):
    def __init__(self, parent=None, settings=None):
        super().__init__(parent)
        self.setWindowTitle(strings.get('settingsMenuTitleText','Settings'))
        self.setModal(True)
        self.resize(520, 380)

        self.settings = dict(settings or {})

        self.setStyleSheet("""
            QDialog {
                background-color: #000000;
            }
            QCheckBox {
                color: white;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border: 2px solid #888;
                border-radius: 4px;
                background: black;
            }
            QCheckBox::indicator:checked {
                background: #4CAF50;
                border: 2px solid #4CAF50;
            }
        """)

        

        # main toggles
        main_keys = ["dark_theme","hacker_font","slower_animations","update_check","impatient","enable_preload"]
        dev_keys  = ["debug_info","i_know_what_im_doing","basic_success_checks"]

        layout = QtWidgets.QVBoxLayout(self)

        # logo row
        logo = self._logo_widget()
        layout.addWidget(logo)

        grid = QtWidgets.QGridLayout()
        self.boxes = {}
        r = 0
        for k in main_keys:
            cb = QtWidgets.QCheckBox(k.replace("_"," ").title())
            cb.setChecked(bool(self.settings.get(k, False)))
            self.boxes[k] = cb
            grid.addWidget(cb, r//2, r%2)
            r += 1
        layout.addLayout(grid)

        layout.addSpacing(8)
        dev_label = QtWidgets.QLabel(strings.get('devSettingsTitle','Developer Settings'))
        dev_label.setStyleSheet("color:#aaa; font-weight:600; margin-top:6px;")
        layout.addWidget(dev_label)

        dev_grid = QtWidgets.QGridLayout()
        for i, k in enumerate(dev_keys):
            cb = QtWidgets.QCheckBox(k.replace("_"," ").title())
            cb.setChecked(bool(self.settings.get(k, False)))
            self.boxes[k] = cb
            dev_grid.addWidget(cb, i//2, i%2)
        layout.addLayout(dev_grid)

        layout.addStretch(1)
        btns = QtWidgets.QHBoxLayout()
        btnCancel = QtWidgets.QPushButton("Cancel")
        btnApply  = QtWidgets.QPushButton(strings.get('applyText','Apply'))
        btns.addStretch(1); btns.addWidget(btnCancel); btns.addWidget(btnApply)
        layout.addLayout(btns)

        btnCancel.clicked.connect(self.reject)
        btnApply.clicked.connect(self._apply)

    def _apply(self):
        for k, cb in self.boxes.items():
            self.settings[k] = bool(cb.isChecked())
        save_settings(self.settings)
        self.accept()

    def _logo_widget(self):
        w = QtWidgets.QFrame()
        h = QtWidgets.QHBoxLayout(w)
        pic = QtWidgets.QLabel()
        pic.setFixedSize(40,40)
        pth = _find_logo()
        if pth:
            pm = QtGui.QPixmap(pth).scaled(40,40, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
            pic.setPixmap(pm)
        else:
            # draw placeholder gradient
            pm = QtGui.QPixmap(40,40); pm.fill(QtCore.Qt.transparent)
            qp = QtGui.QPainter(pm)
            grad = QtGui.QLinearGradient(0,0,40,40)
            grad.setColorAt(0, QtGui.QColor(124,77,255))
            grad.setColorAt(1, QtGui.QColor(3,218,198))
            qp.setBrush(grad); qp.setPen(QtCore.Qt.NoPen); qp.drawRoundedRect(0,0,40,40,8,8); qp.end()
            pic.setPixmap(pm)
        title = QtWidgets.QLabel(strings.get('settingsMenuTitleText','Settings'))
        title.setStyleSheet("font-size:18px; font-weight:700;")
        h.addWidget(pic); h.addSpacing(10); h.addWidget(title); h.addStretch(1)
        return w

# ------------ main window ------------
class MainWindow(QtWidgets.QMainWindow):
    instance = None  # for set_brand() global bridge

    def __init__(self):
        super().__init__()
        MainWindow.instance = self

        self.setWindowTitle("nPhoneKIT")
        self.resize(1550, 860)
        self.pool = QtCore.QThreadPool.globalInstance()

        # theming
        self._settings = load_settings()
        self.apply_theme(self._settings.get("dark_theme", True), self._settings.get("hacker_font", False))

        # layout: splitter (left content tabs, right output console)
        splitter = QtWidgets.QSplitter()
        splitter.setOrientation(QtCore.Qt.Horizontal)
        self.setCentralWidget(splitter)

        # left: brand tabs + actions
        left = QtWidgets.QWidget(); lyt = QtWidgets.QVBoxLayout(left); lyt.setContentsMargins(16,16,16,16); lyt.setSpacing(12)
        self.tabs = QtWidgets.QTabWidget()
        # fancy round tabs
        self.tabs.setDocumentMode(True)
        self.tabs.setTabPosition(QtWidgets.QTabWidget.North)
        lyt.addWidget(self.tabs)
        splitter.addWidget(left)

        # right: header + output
        right = QtWidgets.QWidget(); rlyt = QtWidgets.QVBoxLayout(right); rlyt.setContentsMargins(0,0,12,12); rlyt.setSpacing(10)
        header = self._build_header()
        rlyt.addWidget(header)
        self.output = QtWidgets.QTextEdit()
        self.output.setReadOnly(True)
        rlyt.addWidget(self.output, 1)
        splitter.addWidget(right)

        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 1)

        # redirect stdout/stderr
        self._redirector = QtRedirectText(self.output)
        sys.stdout = self._redirector
        sys.stderr = self._redirector

        # busy overlay on whole window
        self.overlay = BusyOverlay(self)

        # tabs and buttons
        self._brand_index = {}
        self._build_brand_tabs()

        # welcome text
        print(strings.get('nPhoneKITwelcome', 'Welcome to nPhoneKIT').format(version=version))
        print(strings.get('newIn1.3.2', ''))

        # window animation
        self._fade_in()

    def resizeEvent(self, e):
        super().resizeEvent(e)
        if self.overlay and self.overlay.isVisible():
            self.overlay.setGeometry(self.rect())

    # ----- UI construction helpers -----
    def _build_header(self):
        bar = QtWidgets.QFrame()
        bar.setObjectName("Header")
        hlay = QtWidgets.QHBoxLayout(bar); hlay.setContentsMargins(16,10,16,10)
        # logo
        logo = QtWidgets.QLabel(); logo.setFixedSize(36,36)
        pth = _find_logo()
        if pth:
            pm = QtGui.QPixmap(pth).scaled(36,36, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
            logo.setPixmap(pm)
        else:
            pm = QtGui.QPixmap(36,36); pm.fill(QtCore.Qt.transparent)
            qp = QtGui.QPainter(pm)
            grad = QtGui.QLinearGradient(0,0,36,36)
            grad.setColorAt(0, QtGui.QColor(124,77,255))
            grad.setColorAt(1, QtGui.QColor(3,218,198))
            qp.setBrush(grad); qp.setPen(QtCore.Qt.NoPen); qp.drawRoundedRect(0,0,36,36,8,8); qp.end()
            logo.setPixmap(pm)
        title = QtWidgets.QLabel("nPhoneKIT")
        title.setObjectName("AppTitle")
        subtitle = QtWidgets.QLabel(f"v{version}")
        subtitle.setStyleSheet("color: rgba(255,255,255,0.85); font-size:13px;")

        tbox = QtWidgets.QVBoxLayout(); tbox.setSpacing(0)
        tbox.addWidget(title); tbox.addWidget(subtitle)

        btnSettings = QtWidgets.QPushButton(strings.get('settingsMenuTitleText','Settings'))
        btnSettings.clicked.connect(self.open_settings)

        hlay.addWidget(logo); hlay.addSpacing(10); hlay.addLayout(tbox); hlay.addStretch(1); hlay.addWidget(btnSettings)
        return bar

    def _brand_tab(self, title, actions):
        w = QtWidgets.QWidget(); grid = QtWidgets.QGridLayout(w)
        grid.setContentsMargins(8,8,8,8); grid.setHorizontalSpacing(12); grid.setVerticalSpacing(12)
        # build pretty buttons
        font_big = QtGui.QFont(); font_big.setPointSize(12); font_big.setBold(True)
        for i, (label, tooltip, fn) in enumerate(actions):
            btn = QtWidgets.QPushButton(label)
            btn.setToolTip(tooltip)
            btn.setMinimumHeight(44)
            btn.clicked.connect(partial(self.run_task, fn))
            # put into a card-like holder
            card = QtWidgets.QFrame(); card.setStyleSheet("QFrame { background: rgba(255,255,255,0.03); border: 1px solid #2A2A2A; border-radius: 12px; }")
            v = QtWidgets.QVBoxLayout(card); v.setContentsMargins(10,10,10,10)
            btn.setFont(font_big)
            v.addWidget(btn)
            grid.addWidget(card, i//2, i%2)
        return w

    def _build_brand_tabs(self):
        # actions must call your existing backend functions
        samsung_actions = [
            (strings.get('frpUnlock2024','FRP Unlock 2024 🔓'), strings.get('frpUnlock2024info',''), frp_unlock_2024),
            (strings.get('frpUnlock2022','FRP Unlock 2022 ⛓️'), strings.get('frpUnlock2022info',''), frp_unlock_aug2022_to_dec2022),
            (strings.get('frpUnlockPre2022','FRP Unlock pre-2022 🔓'), strings.get('frpUnlockPre2022info',''), frp_unlock_pre_aug2022),
            (strings.get('getVerInfo','Get Version Info 🧾'), strings.get('getVerInfoTooltip',''), verinfo),
            (strings.get('crashReboot','Crash/Reboot ⚡'), strings.get('crashRebootInfo',''), reboot_sam),
            (strings.get('samRebootDownloadMode','Reboot to Download ⬇️'), strings.get('samRebootDownloadModeInfo',''), reboot_download_sam),
            (strings.get('samWifitest','WIFITEST 🔧'), strings.get('samWifitestInfo',''), wifitest),
            (strings.get('samImeiCheck','IMEI Check 🔍'), strings.get('samImeiCheckInfo',''), imeicheck),
            (strings.get('samRemoveBloat','Remove Bloat 🧹'), strings.get('samRemoveBloatInfo',''), bloatRemove),
        ]
        lg_actions = [
            (strings.get('lgScreenUnlockLabel','LG Screen Unlock 🔓'), strings.get('lgScreenUnlockTooltip',''), LG_screen_unlock),
        ]
        mtk_actions = [
            (strings.get('mtkClientLabel','MTK Client GUI 🚀'), strings.get('mtkClientTooltip',''), mtkclient),
        ]
        android_actions = [
            (strings.get('crashReboot','Crash/Reboot ⚡'), strings.get('crashRebootInfo',''), reboot),
        ]

        tabspec = [
            (strings.get('brandSamsung','Samsung'), samsung_actions),
            (strings.get('brandLg','LG'), lg_actions),
            (strings.get('brandMediatek','MediaTek'), mtk_actions),
            (strings.get('brandAndroid','Android'), android_actions),
        ]
        self.tabs.clear()
        self._brand_index.clear()
        for i, (title, acts) in enumerate(tabspec):
            page = self._brand_tab(title, acts)
            self.tabs.addTab(page, title)
            self._brand_index[title] = i

        # select default
        want = "Samsung"
        self.set_brand(want)

    # ----- public: programmatic brand selection -----
    def set_brand(self, name):
        idx = self._brand_index.get(name)
        if idx is not None:
            self.tabs.setCurrentIndex(idx)

    # ----- run task with spinner & thread -----
    def run_task(self, fn):
        self.overlay.start()
        worker = Worker(fn)
        worker.signals.finished.connect(self.overlay.stop)
        worker.signals.error.connect(lambda e: print(f" FAIL {e}"))
        self.pool.start(worker)

    # ----- settings -----
    def open_settings(self):
        dlg = SettingsDialog(self, settings=self._settings)
        if dlg.exec_() == QtWidgets.QDialog.Accepted:
            self._settings = dlg.settings
            # immediately apply theme if toggled
            self.apply_theme(self._settings.get("dark_theme", True), self._settings.get("hacker_font", False))

    def apply_theme(self, dark, hacker):
        self.setStyleSheet(_material_qss(dark=dark, hacker=hacker))

    # ----- nice fade-in -----
    def _fade_in(self):
        self.setWindowOpacity(0.0)
        anim = QtCore.QPropertyAnimation(self, b"windowOpacity", self)
        anim.setDuration(400 if not self._settings.get("slower_animations", False) else 900)
        anim.setStartValue(0.0); anim.setEndValue(1.0); anim.start(QtCore.QAbstractAnimation.DeleteWhenStopped)

current_brand = strings.get('brandCurrent', 'Samsung')

def select_brand(name):
    global current_brand
    current_brand = name
    if MainWindow.instance:
        MainWindow.instance.set_brand(name)

def set_brand(name):
    select_brand(name)

# --- Instant / Tunable Tooltips for PyQt5 ---
class InstantTooltips(QtCore.QObject):
    """
    Global tooltip accelerator.
    - delay_ms: how long to wait before showing (0 = instant)
    - hide_ms: auto-hide after N ms (<=0 disables auto-hide)
    """
    def __init__(self, delay_ms=100, hide_ms=0, parent=None):
        super().__init__(parent)
        self.delay_ms = max(0, int(delay_ms))
        self.hide_ms = int(hide_ms)
        self._timer = QtCore.QTimer(self)
        self._timer.setSingleShot(True)
        self._pending = None  # (global_pos, widget, text)

    def eventFilter(self, obj, event):
        et = event.type()
        if et == QtCore.QEvent.ToolTip:
            text = obj.toolTip() if hasattr(obj, "toolTip") else ""
            if not text:
                QtWidgets.QToolTip.hideText()
                return True
            pos = obj.mapToGlobal(event.pos())
            if self.delay_ms == 0:
                QtWidgets.QToolTip.showText(pos, text, obj)
                if self.hide_ms > 0:
                    QtCore.QTimer.singleShot(self.hide_ms, QtWidgets.QToolTip.hideText)
            else:
                self._pending = (pos, obj, text)
                self._timer.stop()
                self._timer.timeout.disconnect() if self._timer.receivers(self._timer.timeout) else None
                self._timer.timeout.connect(self._show_pending)
                self._timer.start(self.delay_ms)
            return True  # we handled it (prevents default slow tooltip)
        elif et in (QtCore.QEvent.Leave, QtCore.QEvent.FocusOut):
            QtWidgets.QToolTip.hideText()
        return False

    def _show_pending(self):
        if not self._pending:
            return
        pos, w, text = self._pending
        self._pending = None
        if w and w.isVisible():
            QtWidgets.QToolTip.showText(pos, text, w)
            if self.hide_ms > 0:
                QtCore.QTimer.singleShot(self.hide_ms, QtWidgets.QToolTip.hideText)

# ------------- entry point -------------
def main():
    app = QtWidgets.QApplication(sys.argv)
    # apply tooltip palette for visibility
    pal = app.palette()
    pal.setColor(QtGui.QPalette.ToolTipBase, QtGui.QColor(42,42,42))
    pal.setColor(QtGui.QPalette.ToolTipText, QtGui.QColor(240,240,240))
    app.setPalette(pal)

    fast_tips = InstantTooltips(delay_ms=1, hide_ms=299000)
    app.installEventFilter(fast_tips)

    win = MainWindow()
    win.show()
    sys.exit(app.exec_())

# ===================================
#  Preparing to start the app
# ===================================

def is_root():
    if os_config == "WINDOWS":  # Windows
        try:
            import ctypes
            return ctypes.windll.shell32.IsUserAnAdmin() != 0
        except Exception:
            return False
    elif os_config == "LINUX":  # POSIX (Linux, macOS, etc)
        return os.geteuid() == 0
    
if not is_root():
    root = tk.Tk()
    root.withdraw()

    messagebox.showwarning("nPhoneKIT", strings['sudoReqdError'])
    sys.exit(1)

if update_check:
    check_for_update()

serman1 = SerialManager()

def preload_thread():
    asyncio.run(preload_samsung_modem(serman1))

threading.Thread(target=preload_thread, daemon=True).start()

if __name__ == "__main__": # If directly opened, start nPhoneKIT
    ttthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), "NOT_First", "NOT_First", "Success", False))
    ttthread.start() # Sends basic, anonymized success_checks info with only the model number.
    rt() # Flush the buffer from previous runs of nPhoneKIT just in case
    main() # Start the main GUI (with a cool animation)
