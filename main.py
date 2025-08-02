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
import xml.etree.ElementTree as ET

## nPhoneKIT permissions (these are the things that nPhoneKIT is capable of doing):

# Communicate with USB devices using ADB, MTP, and AT commands.
# Communicate with external servers to verify whether an action worked or not.
# Open a new tab in the default browser
# Checking and getting basic information about the current system

version = "1.3.2"

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
dark_theme = settings["dark_theme"]
hacker_font = settings["hacker_font"]
slower_animations = settings["slower_animations"]
update_check = settings["update_check"]
impatient = settings["impatient"]
enable_preload = settings["enable_preload"]
debug_info = settings["debug_info"]
i_know_what_im_doing = settings["i_know_what_im_doing"]
basic_success_checks = settings["basic_success_checks"]

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
    win.title(strings["settingsMenuTitleText"])

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
        dev_win.title(strings["devSettingsTitle"])
        dev_vars = {}

        for i, key in enumerate(["debug_info", "i_know_what_im_doing", "basic_success_checks"]):
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

        tk.Button(dev_win, text=strings["applyText"], command=apply_dev).grid(row=i+1, columnspan=2)

    def apply_main():
        for k, v in vars.items():
            settings[k] = v.get()
        save_settings(settings)
        win.destroy()

    tk.Button(win, text=strings["applyText"], command=apply_main).grid(row=row, column=0)
    tk.Button(win, text=strings["devSettingsTitle"], command=open_dev_settings, font=("Segoe UI", 5), width=20, height=1).grid(row=row+1, column=0)

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
                print(strings["noDeviceSermanError"]) 
        elif self.port:
            try:
                self.ser = serial.Serial(self.port, self.baud, timeout=2) # Save the port for use with the rest of the class
                time.sleep(0.5)
                if debug_info:
                    print(f"{strings["sermanConnectedPort"]}{self.port}")
            except serial.SerialException as e:
                raise RuntimeError(f"{strings["sermanOpeningPortError"]}{self.port}: {e}")

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
                    print(strings["noDeviceGenericError"])
            else:
                print(strings["noDeviceGenericError"])
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
            raise RuntimeError(strings["sermanWindowsOsError"])

        self.debug = debug
        self.baud = baud
        self.ser = None

        # allow override, else auto-detect
        self.port = port or self.detect_port()
        if not self.port:
            raise RuntimeError(strings["sermanNoComPort"])

        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2)
            time.sleep(0.5)
            if self.debug:
                print(f"{strings["sermanConnectedPort"]}{self.port} @ {self.baud} baud")
        except serial.SerialException as e:
            raise RuntimeError(f"{strings["sermanOpeningPortError"]}{self.port}: {e}")
        
    def reset(self):
        self.__init__()

    def detect_port(self) -> str:
        """Return the first COM* port or None."""
        ports = list_ports.comports()
        if self.debug:
            print(f"{strings["sermanWinAvailablePorts"]}{[p.device for p in ports]}")
        for p in ports:
            if p.device.upper().startswith("COM"):
                if self.debug:
                    print(f"{strings["sermanWinDev"]}{p.device}")
                return p.device
        return None

    def send(self, command: str, wait: float = 0.1) -> str:
        """
        Send a command and collect all response lines.
        :param command: Text/AT command to send.
        :param wait: Seconds to pause before reading.
        """
        if not self.ser or not self.ser.is_open:
            raise RuntimeError(strings["serPortNotOpen"])

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
                print(strings["sermanWinConClosed"])

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
                    print(strings["deviceConCheckNotPlugged"])
    
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
                print(strings["samPreloadUsbDetected"])
            # If a Samsung device is plugged in, preload its modem using the below commands
            set_brand("Samsung") # For convenience, auto-select the SAMSUNG menu in the nPhoneKIT GUI
            serman2.send("AT+SWATD=0")  # Send without await since it's serial and blocking
            serman2.send("AT+ACTIVATE=0,0,0") # This and the above command do the same thing as modemUnlock("SAMSUNG"), except without infinitely waiting for preload_done, since modemUnlock uses the AT class which will follow preload_done
            if debug_info:
                print(strings["samPreloadComplete"])
            preload_error = False
        else:
            if debug_info:
                print(strings["samNoUsbFound"])
            enable_preload = False
            preload_error = True

    except Exception as e:
        if debug_info:
            print(strings["samPreloadError"], e) # Usually error, but works most of the time reguardless.
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

            latest_version_raw = data["tag_name"]
            latest_version = data["tag_name"].lstrip("v")

            # If the tag is different then the current version, assume it's newer, and prompt update.

            # Based on the unicode "v", depending on whether it's normal or U+2174, prompt for normal update and FORCE for critical update

            if latest_version != version:
                if "ⅴ" in latest_version_raw:
                    messagebox.showinfo(
                        strings["updateReqd"],
                        strings["updateReqdString"].format(version=version, latest_version=latest_version)
                    )
                    sys.exit(0) # Exit and do not let user use nPhoneKIT if the update is REQUIRED or critical
                else:   
                    messagebox.showinfo(
                        strings["updateAvail"],
                        strings["updateAvailString"].format(version=version, latest_version=latest_version)
                    )
    except Exception as e:
        print(strings["updateCheckFailed"])

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
    show_messagebox_at(500, 200, "nPhoneKIT", strings["mtpMenu"])
    # Show user instructions to enable MTP mode

def adbMenu():
    show_messagebox_at(500, 200, "nPhoneKIT", strings["adbMenu"])
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
    print(strings["getVerInfo"], end="")
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output

    if info == "Fail":
        print(strings["deviceCheckPluggedIn2"])
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

        show_messagebox_at(500, 200, "nPhoneKIT", strings["misuseFrpGuidance"])

        print(strings["attemptingEnableAdb"], end="")

        show_messagebox_at(500, 200, "nPhoneKIT", strings["frpUnlockStepsPre2022"])

        for command in ATcommands:
            AT.send(command)

        output = readOutput("AT")

        if "error" in output.lower():
            print(strings["failText"])
            print(strings["frpNotCompatible"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Pre_2022", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            print(strings["okText"])
            print(strings["runUnlock"], end="")
            show_messagebox_at(500, 200, "nPhoneKIT", strings["usbDebuggingPromptCheck"])
            for command in ADBcommands:
                ADB.send(command)
            print(strings["okText"])
            print(strings["unlockSuccess"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Pre_2022", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

def frp_unlock_aug2022_to_dec2022(): # FRP unlock for aug2022-dec2022 security patch update
    print(strings["getVerInfo"], end="")
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output

    if info == "Fail":
        print(strings["deviceCheckPluggedIn2"])
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

        show_messagebox_at(500, 200, "nPhoneKIT", strings["misuseFrpGuidance2022"])

        print(strings["attemptingEnableAdb"], end="")

        show_messagebox_at(500, 200, "nPhoneKIT", strings["frpUnlockSteps2022"])

        for command in commands:
            AT.send(command)

        output = readOutput("AT")

        if "error" in output.lower():
            print(strings["failText"])
            print(strings["frpNotCompatible"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Aug_To_Dec_2022", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            print(strings["okText"])
            print(strings["runUnlock"], end="")
            show_messagebox_at(500, 200, "nPhoneKIT", strings["usbDebuggingPromptCheck"])
            for command in ADBcommands:
                ADB.send(command)
            print(strings["okText"])
            print(strings["unlockSuccess"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_Aug_To_Dec_2022", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

def frp_unlock_2024(): # FRP unlock for early 2024-ish security patch update
    print(strings["getVerInfo"], end="")
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output

    if info == "Fail":
        print(strings["deviceCheckPluggedIn2"])
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

        show_messagebox_at(500, 200, "nPhoneKIT", strings["misuseFrpGuidance2024"])

        print(strings["attemptingEnableAdb"], end="")

        show_messagebox_at(500, 200, "nPhoneKIT", strings["frpUnlockSteps2024"])

        for command in commands:
            AT.send(command)

        output = readOutput("AT")

        if "error" in output.lower():
            print(strings["failText"])
            print(strings["frpNotCompatible"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "FRP_Unlock_2024", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            print(strings["okText"])
            print(strings["runUnlock"], end="")
            show_messagebox_at(500, 200, "nPhoneKIT", strings["usbDebuggingPromptCheck"])
            for command in ADBcommands:
                ADB.send(command)
            print(strings["okText"])
            print(strings["unlockSuccess"])
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
        print(strings["deviceNotSupportedUniversal"])

def LG_screen_unlock(): # Screen unlock on supported LG devices *untested*
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info) # Extract only the model no. from the output (may not work)

    show_messagebox_at(500, 200, "nPhoneKIT", strings["lgScreenUnlockSupportedDevs"])
    print(strings["lgRunningScreenUnlock"], end="")
    # Prepare phone for unlock
    show_messagebox_at(600, 100, "nPhoneKIT", strings["lgScreenUnlockSteps"])
    
    time.sleep(1)
    if AT.usbswitch("-l", "LG Screen Unlock"):
        rt() # Flush the output buffer
        AT.send('AT%KEYLOCK=0') # This AT command SHOULD unlock the screen instantly. (yes, one command.)
        with open("tmp_output.txt", "r") as f:
            output = f.read()
        # debug only: print("\n\nOutput: \n\n" + output + "\n\n")
        if "error" in output or "Error" in output:
            print(strings["failText"] + "\n")
            print(strings["lgScreenUnlockError"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "LG_Screen_Unlock", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.
        else:
            rt()
            print(strings["okText"] + "\n")
            print(strings["lgScreenUnlockSuccess"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "LG_Screen_Unlock", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number. This is so we know what devices are compatible with which unlocks.

# ==============================================
#  Simple functions that do stuff to the device
# ==============================================

def verinfo(gui=True, showtext=True): # Get version info on the device. Pretty simple. (not simple, this has taken me hours.)
    if gui:
        if enable_preload: # Skip all the nonsense and cut straight to the action, no "testAT" nonsense. We're prioritizing speed.
            print(strings["getVerInfo"], end="")
            AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
            output = readOutput("AT") # Output is retrieved from the command
            if output == "" or output == None:
                AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
                output = readOutput("AT")
                if output == "" or output == None:
                    print(strings["failText"])
                    print(strings["verInfoCheckConn"])
                    model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                    tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Fail"))
                    tthread.start() # Sends basic, anonymized success_checks info with only the model number.
            else:
                print(strings["okText"])
                model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                tthread.start() # Sends basic, anonymized success_checks info with only the model number.
            output = parse_devconinfo(output) # Make the output actually readable
            print(output) # Print the version info to the output box
        else: 
            print(strings["getVerInfo"], end="")
            if testAT(True, text=strings["getVerInfo"]): # We should verify AT is working before running the below code (testAT is deprecated)
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
                                print(strings["failText"])
                                print(strings["verInfoCheckConn"])
                            else:
                                output = parse_devconinfo(output) # Make the output actually readable
                                model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                                tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                                tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                                print(strings["okText"])
                        except Exception:
                            print(strings["verInfoCheckConn"])
                    else:
                        output = parse_devconinfo(output) # Make the output actually readable
                        model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                        tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                        print(strings["okText"])
                        print(output) # Print the version info to the output box
                else:
                    output = parse_devconinfo(output) # Make the output actually readable
                    model = re.search(r'Model:\s*(\S+)', output) # Extract only the model no. from the output
                    tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "VersionInfo", "Success"))
                    tthread.start() # Sends basic, anonymized success_checks info with only the model number.
                    print(strings["okText"])
                    print(output) # Print the version info to the output box
    else:
        #print(strings["getVerInfo"], end="")
        if testAT(True, text=strings["getVerInfo"]): # We should verify AT is working before running the below code (deprecated)
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
                        print(strings["failText"])
                else:
                    if showtext:
                        print(strings["okText"])
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

    print(strings["openingWifitest"], end="")
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
        print(strings["okText"])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "WIFITEST", "Success"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    else:
        print(strings["failText"])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "WIFITEST", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def reboot(): # Crash an android phone to reboot
    print(strings["crashingToReboot"], end="")
    MTPmenu()
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info)
    rt()
    try:
        AT.send("AT+CFUN=1,1") # Crashes the phone immediately.
    except Exception as e:
        if "disconnected" in str(e):
            print(strings["okText"]) # Error opening serial means that the command worked, because it reset the phone before it could give a response.
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    output = readOutput("AT")
    if "OK" in output:
        print(strings["failText"])
        print(strings["crashRebootFailed"])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def reboot_sam(): # Crash a Samsung phone to reboot
    print(strings["crashingToReboot"], end="")
    MTPmenu()
    modemUnlock("SAMSUNG", True)
    info = verinfo(False)
    model = re.search(r'Model:\s*(\S+)', info)
    rt()
    try:
        AT.send("AT+CFUN=1,1") # Crashes the phone immediately.
    except Exception as e:
        if "disconnected" in str(e):
            print(strings["okText"]) # Error opening serial means that the command worked, because it reset the phone before it could give a response.
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT_SAM", "Success"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number.
    output = readOutput("AT")
    if "OK" in output:
        print(strings["failText"])
        print(strings["crashRebootFailed"])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), model, "REBOOT_SAM", "Fail"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def bloatRemove():
    print(strings["uninstallingPackages"], end="")
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
            print(strings["failText"])
            print(strings["devNotConnectedOrOtherErr"])
            tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), None, "DEBLOAT_SAM", "Fail"))
            tthread.start() # Sends basic, anonymized success_checks info with only the model number.
            break
    if "Success" in readOutput("ADB") or "[n" in readOutput("ADB") or "age:" in readOutput("ADB"):
        print(strings["okText"])
        print(strings["debloatSucceeded"])
        tthread = threading.Thread(target = success_checks, args = (get_public_hardware_uuid(), None, "DEBLOAT_SAM", "Success"))
        tthread.start() # Sends basic, anonymized success_checks info with only the model number.

def reboot_download_sam(): # Reboot Samsung device to download mode
    print(strings["rebootingDownloadMode"], end="")
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
        messagebox.showinfo("nPhoneKIT", strings["imeiCheckGuide"])
        if os_config == "WINDOWS":
            webbrowser.open_new_tab(f"https://www.imei.info/services/blacklist-simple/samsung/check-free/?imei={str(imei)}")
        elif os_config == "LINUX":
            url = f"https://www.imei.info/services/blacklist-simple/samsung/check-free/?imei={str(imei)}"
            original_user = os.environ.get("SUDO_USER", "yourusername")  # linux is complicated :/
            cmd = f'su - {original_user} -c "DISPLAY=$DISPLAY DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS xdg-open \\"{url}\\""'
            os.system(cmd)
        print(strings["imeiChecked"])
    else:
        print(strings["imeiNotFound"])

def mtkclient():
    if os_config == "WINDOWS":
        os.system('pip install -r deps/mtkclient/requirements.txt')
        os.system('python ./deps/mtkclient/mtk_gui.py')
    elif os_config == "LINUX":
        #os.system('sudo pip install --no-deps statsd scrypt repoze.lru keystone-engine fusepy aniso8601 Yappi wrapt werkzeug WebOb vine unicorn tzdata testtools shiboken6 Routes rfc3986 pyusb pyflakes pycryptodomex pycryptodome pycodestyle psutil prometheus-client PrettyTable pbr PasteDeploy Paste netaddr msgpack mccabe itsdangerous iso8601 greenlet elementpath dnspython capstone cachetools blinker xmlschema testscenarios testresources stevedore SQLAlchemy PySide6-Essentials oslo.i18n oslo.context os-service-types Flask flake8 eventlet debtcollector amqp PySide6-Addons pysaml2 oslo.utils oslo.config kombu keystoneauth1 futurist Flask-RESTful dogpile.cache alembic pyside6 oslo.serialization oslo.middleware oslo.db oslo.concurrency python-keystoneclient pycadf osprofiler oslo.policy oslo.log oslo.upgradecheck oslo.service oslo.metrics oslo.cache oslo.messaging keystonemiddleware keystone --break-system-packages')
        #os.system('sudo python3 deps/mtkclient/mtk_gui.py')
        os.system('sudo apt install libxcb-cursor0')
        os.system("sudo bash -c 'source ./deps/venv/bin/activate && python3 ./deps/mtkclient/mtk_gui.py'")

# ===================================
#  Tkinter GUI Stuff
# ===================================

class RedirectText: # Redirect all print() statements to the output terminal
    def __init__(self, text_ctrl):
        self.output = text_ctrl
        # compile once for speed
        self.pattern = re.compile(r"( FAIL| OK)")

    def write(self, string):
        idx = 0
        for m in self.pattern.finditer(string):
            # insert text before match
            self.output.insert("end", string[idx:m.start()])
            token = m.group(1)
            # choose tag based on token
            tag = "fail" if token.strip() == "FAIL" else "ok"
            self.output.insert("end", token, tag)
            idx = m.end()
        # insert any trailing text
        self.output.insert("end", string[idx:])
        self.output.see("end")

    def flush(self):
        pass

current_brand = strings["brandCurrent"]
brand_buttons = {}  
brand_frames = {} 

def select_brand(name): # Choose the brand based on the detected connected device
    global current_brand
    current_brand = name
    for bname, btn in brand_buttons.items():
        btn.config(relief="raised", bg="#cccccc")
    brand_buttons[name].config(relief="sunken", bg="#9999ff")

    for bname, frame in brand_frames.items():
        frame.pack_forget()
    brand_frames[name].pack(fill="y")

def set_brand(name):
    select_brand(name) # Change selected brand from anywhere in the script

def main(): # Tkinter GUI
    root = tk.Tk()
    root.title("nPhoneKIT")
    root.geometry("1550x800")

    emoji_font = font.Font(family="Helvetica", size=10)

    if dark_theme:
        background_color_1 = "#000000"
        background_color_2 = "#444444"
    else:
        background_color_1 = "#dddddd"
        background_color_2 = "#cccccc"

    left_frame = tk.Frame(root, width=1050, bg=background_color_1, bd=0, relief="flat")
    left_frame.pack(side="left", fill="y")

    button_bar = tk.Frame(left_frame, bg=background_color_1)
    button_bar.pack(fill="x")

    for name in [strings["brandSamsung"], strings["brandLg"], strings["brandMediatek"], strings["brandAndroid"]]:
        btn = tk.Button(button_bar, text=name, font=("Helvetica", 20), width=15,
                        command=lambda n=name: select_brand(n), bg=background_color_2)
        btn.pack(side="left", padx=10, pady=10)
        brand_buttons[name] = btn

    content_area = tk.Frame(left_frame, bg=background_color_1, bd=0, relief="flat")
    content_area.pack(fill="both", expand=True)

    samsung_frame_A = tk.Frame(content_area, bg=background_color_1, bd=0, relief="flat")
    lg_frame = tk.Frame(content_area, bg=background_color_1, bd=0, relief="flat")
    mediatek_frame = tk.Frame(content_area, bg=background_color_1, bd=0, relief="flat")
    general_frame = tk.Frame(content_area, bg=background_color_1, bd=0, relief="flat")

    # Output frame
    right_frame = tk.Frame(root, width=200, bd=0, relief="flat")
    right_frame.pack(side="right", fill="both")
    if dark_theme:
        top_outputbar = tk.Frame(right_frame, bg="#9191eb", height=100)
    else:
        top_outputbar = tk.Frame(right_frame, bg="#6969f1", height=100)
    top_outputbar.pack(fill="x")
    if dark_theme:
        if hacker_font:
            output_box = tk.Text(right_frame, width=525, wrap="word", bg="#000000", fg="#2FD948", font=("Courier", 15), bd=0, relief="flat", highlightthickness=0)
        else:
            output_box = tk.Text(right_frame, width=525, wrap="word", bg="#000000", fg="#FFFFFF", font=("Courier", 15), bd=0, relief="flat", highlightthickness=0)
    else:
        if hacker_font:
            output_box = tk.Text(right_frame, width=525, wrap="word", bg="#f5f5f5", fg="#2FD948", font=("Courier", 15), bd=0, relief="flat", highlightthickness=0)
        else:
            output_box = tk.Text(right_frame, width=525, wrap="word", bg="#f5f5f5", font=("Courier", 15), bd=0, relief="flat", highlightthickness=0)
    output_box.pack(expand=True, fill="both")
    output_box.tag_configure("fail", foreground="red")
    output_box.tag_configure("ok",  foreground="green")
    
    sys.stdout = RedirectText(output_box)
    sys.stderr = RedirectText(output_box)

    # Header
    if dark_theme:
        tk.Label(top_outputbar, text="nPhoneKIT", bg="#9191eb", fg="black", font=("Helvetica", 24, "bold"), width=10).pack(pady=10, side="left")
    else:
        tk.Label(top_outputbar, text="nPhoneKIT", bg="#6969f1", fg="white", font=("Helvetica", 24, "bold"), width=10).pack(pady=10, side="left")

    tk.Button(top_outputbar, text="Settings", command=open_settings_window).pack(side="left", padx=(30, 100))

    brand_frames[strings["brandSamsung"]] = samsung_frame_A
    brand_frames[strings["brandMediatek"]] = mediatek_frame
    brand_frames[strings["brandLg"]] = lg_frame
    brand_frames[strings["brandAndroid"]] = general_frame

    emoji_font_bold = font.Font(family="Noto Color Emoji", size=10, weight="bold")
    emoji_font = font.Font(family="Noto Color Emoji", size=10)
    emoji_font_smaller = font.Font(family="Noto Color Emoji", size=8)

    # Tooltip label (hidden by default)
    tooltip = tk.Label(root, text="", bg="yellow", font=("Helvetica", 12), bd=1, relief="solid")
    tooltip.place_forget()

    if slower_animations:
        anim_speed = 8
    else:
        anim_speed = 16

    def animate_window_open(target_width=1700, speed=anim_speed, delay=1):
        def expand():
            nonlocal current_width
            if current_width < target_width:
                current_width += speed
                root.geometry(f"{current_width}x800")  # assuming fixed height
                root.after(delay, expand)
            else:
                root.geometry(f"{target_width}x800")  # ensure final size exact
        current_width = 1
        root.geometry("1x800")  # Start tiny
        root.after(10, expand)  # Start expanding after 10ms

    def show_tooltip(event, text):
        tooltip.config(text=text)
        x = event.x_root - root.winfo_rootx() + 10
        y = event.y_root - root.winfo_rooty() + 10
        tooltip.place(x=x, y=y)
        tooltip.lift()

    def hide_tooltip(event):
        tooltip.place_forget()

    def add_button(frame, text, cmd, prefix, fontType=emoji_font):
        if dark_theme:
            if hacker_font:
                btn = tk.Button(frame, text=text, bg="#444444", fg="#2FD948", command=cmd, font=fontType)
            else:
                btn = tk.Button(frame, text=text, bg="#444444", fg="#cccccc", command=cmd, font=fontType)
        else:
            if hacker_font:
                btn = tk.Button(frame, text=text, bg="#cccccc", fg="#2FD948", command=cmd, font=fontType)
            else:
                btn = tk.Button(frame, text=text, bg="#cccccc", fg="#444444", command=cmd, font=fontType)
        btn.pack(pady=10)
        btn.bind("<Enter>", lambda e, t=text: show_tooltip(e, f"{prefix}"))
        btn.bind("<Leave>", hide_tooltip)


    # For the FRP buttons, the year numbers are Mathematical Sans Unicode, because otherwise Noto Color Emoji would replace the numbers with emojis.
    add_button(samsung_frame_A, strings["frpUnlock2024"], frp_unlock_2024, strings["frpUnlock2024info"], emoji_font_bold)
    add_button(samsung_frame_A, strings["frpUnlock2022"], frp_unlock_aug2022_to_dec2022, strings["frpUnlock2022info"], emoji_font_smaller)
    add_button(samsung_frame_A, strings["frpUnlockPre2022"], frp_unlock_pre_aug2022, strings["frpUnlockPre2022info"], emoji_font_smaller)
    add_button(samsung_frame_A, strings["getVerInfo"], verinfo, strings["getVerInfoTooltip"])
    add_button(samsung_frame_A, strings["crashReboot"], reboot_sam, strings["crashRebootInfo"])
    add_button(samsung_frame_A, strings["samRebootDownloadMode"], reboot_download_sam, strings["samRebootDownloadModeInfo"])
    add_button(samsung_frame_A, strings["samWifitest"], wifitest, strings["samWifitestInfo"])
    add_button(samsung_frame_A, strings["samImeiCheck"], imeicheck, strings["samImeiCheckInfo"])
    add_button(samsung_frame_A, strings["samRemoveBloat"], bloatRemove, strings["samRemoveBloatInfo"])
    

    add_button(lg_frame, strings["lgScreenUnlockLabel"], LG_screen_unlock, strings["lgScreenUnlockTooltip"])
    
    
    add_button(mediatek_frame, strings["mtkClientLabel"], mtkclient, strings["mtkClientTooltip"])


    #add_button(general_frame, "Universal FRP Unlock 🔓", general_frp_unlock, "This is a universal FRP unlock method. It \nshould work on most manufacturers. \n\n=======================================\n\nAttempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically =< Android 8")
    add_button(general_frame, strings["crashReboot"], reboot, strings["crashRebootInfo"])
    #add_button(general_frame, "Remove Carrier Bloatware", bloatRemove, "")

    # Select default brand
    select_brand(current_brand)

    print(strings["nPhoneKITwelcome"].format(version=version))
    print(strings["newIn1.3.2"])
    #print(f"\nNew in v1.3.0: Cleaned up codebase, added built-in settings, made more user-friendly, easier to install, better GUI. \n\nFull Changelog on https://github.com/nlckysolutions/nPhoneKIT/releases/tag/v1.3.0\n\n")
    #print(f"\n\nNew in v1.2.5: Fixed bugs on Windows")
    #print(f"\n\nNew in v1.2.4: Added basic Windows support")
    #print(f"\n\nNew in v1.2.1: Made all AT commands 10-20x faster by including pySerial in main.py")
    #print(f"\n\nNew in v1.2.0: Confirmed WIFITEST and VER_INFO support for S24 Series\n\n")

    animate_window_open()

    root.mainloop()

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

    messagebox.showwarning("nPhoneKIT", strings["sudoReqdError"])
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