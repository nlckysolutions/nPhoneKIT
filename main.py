import time
import os
import tkinter as tk
from tkinter import messagebox
from tkinter import font
from pathlib import Path
import sys
import io
import re
import subprocess
import serial
import platform
import glob
import asyncio
import threading
import urllib.request
import json

version = "1.2.2"

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


# CONFIG

# Basic User-Friendly Config

os_config = "LINUX" # Change based on your OS to "WINDOWS" or "LINUX"
dark_theme = True # If True, changes the GUI to a "dark mode, which is fully black to reduce power on OLED screens
hacker_font = True # If True, changes the output box font to green. Works best with dark_theme
slower_animations = False # If True, slows down all animations to make the GUI more... dramatic?
update_check = True # If True, nPhoneKIT will automatically check for updates and prompt the user if there are any.

# Speed Related Controls

impatient = False # If True, shows the ETA for every single command/action
enable_preload = True # If True, silently preloads (modem unlocks) all Samsung devices that are connected. They must be connected and have ALLOW ACCESS TO PHONE DATA allowed BEFORE starting the script even. This can sometimes speed up commands by 10 seconds+

# Debug / Dev Stuff

debug_info = False # Prints debug info directly from the SerialManager class. Not needed, and breaks the minimalistic flow
i_know_what_im_doing = False # If True, dialog boxes prompting user to enable MTP and ADB are not shown. If you don't know what that means, you shouldn't enable this feature.




# ============================================================================= #
# You shouldn't edit anything below this line unless you know what you're doing #
# ============================================================================= #

preload_done = threading.Event()

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

class SerialManager:
    def __init__(self, baud=115200):
        self.port = self.detect_port()
        self.baud = baud
        self.ser = None

        if not self.port:
            if debug_info:
                print("NO DEVICE CONNECTED, CANNOT USE.")
        elif self.port:
            try:
                self.ser = serial.Serial(self.port, self.baud, timeout=2)
                time.sleep(0.5)
                if debug_info:
                    print(f"[SerialManager] Connected to {self.port}")
            except serial.SerialException as e:
                raise RuntimeError(f"❌ Error opening serial port {self.port}: {e}")

    def detect_port(self):
        system = platform.system()

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
                    print("Error: Your device is not connected!")
            else:
                print("Error: Your device is not connected!")
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

class AT:
    #def __init__(self):
    #    #self.usb_device = "/dev/ttyACM0" # cannot be changed
    #    # Not needed anymore, usbsend.py will auto select based on OS

    def send(command):
        serman = SerialManager() # Making usbsend.py into a built-in class improves command speed by 10-20x, and improves multi-OS compatibility
        rt()
        #if os_config == "LINUX":
        #    os.system(f"sudo bash -c 'sudo python3 -u ./deps/usbsend.py \"{command}\" > tmp_output.txt 2>&1'")
        #elif os_config == "WINDOWS":
        #    with open('tmp_output.txt', 'w') as f:
        #        subprocess.run(['python', '-u', 'deps/usbsend.py', command], stdout=f, stderr=subprocess.STDOUT)
        #elif os_config == "MAC":
        #    print("MACOS devices are not supported!")
        #time.sleep(0.5)
        if enable_preload:
            preload_done.wait()
        with open("tmp_output.txt", "w", encoding="utf-8") as f:
            f.write(serman.send(command))
    
    def usbswitch(arg, action):
        # Later, add logic to allow switching of device interface to AT, for more compatibility.
        return True
    
class ADB:
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
    if not enable_preload:
        preload_done.set()
        return

    try:
        system = platform.system()
        output = ""

        if system == "Linux":
            output = subprocess.check_output(['lsusb']).decode().lower()
        elif system == "Darwin":
            output = subprocess.check_output(['system_profiler', 'SPUSBDataType']).decode().lower()
        elif system == "Windows":
            output = subprocess.check_output(['powershell', 'Get-PnpDevice']).decode().lower()

        if "samsung" in output:
            if debug_info:
                print("[🌀] Samsung USB detected. Preloading...")
            set_brand("Samsung") # For convenience, auto-select the SAMSUNG menu in the nPhoneKIT GUI
            serman2.send("AT+SWATD=0")  # Send without await since it's serial and blocking
            serman2.send("AT+ACTIVATE=0,0,0") # This and the above command do the same thing as modemUnlock("SAMSUNG"), except without infinitely waiting for preload_done, since modemUnlock uses the AT class which will follow preload_done
            if debug_info:
                print("[✅] Preload complete.")
        else:
            if debug_info:
                print("[⚠️] No Samsung USB found. Skipping preload.")

    except Exception as e:
        if debug_info:
            print("[❌] Preload error:", e)

    preload_done.set()


# Check for updates

def check_for_update():
    try:
        # Replace with your actual repo
        repo = "nlckysolutions/nPhoneKIT"
        url = f"https://api.github.com/repos/{repo}/releases/latest"

        with urllib.request.urlopen(url) as response:
            data = json.loads(response.read().decode())
            latest_version = data["tag_name"].lstrip("v")

            if latest_version != version:
                messagebox.showinfo(
                    "Update Available",
                    f"A new version of nPhoneKIT is available!\n\nCurrent: v{version}\nLatest: v{latest_version}\n\nVisit GitHub to update."
                )
    except Exception as e:
        print(f"[Warning] Could not check for updates, check your internet connection?")


# =============================================
#  Different instructions for the user
# =============================================

def MTPmenu():
    #show_messagebox_at(500, 200, "nPhoneKIT", "Steps to continue (PLEASE READ CAREFULLY, THEN CLOSE THE MESSAGE BOX.):\n\n1. Open the dialer on your phone. After opening the dialer, \nplease continue by typing in the following (without quotes): '*#0*#', and press CALL.\n A service/test menu should be opened. If not, you can also try '*#*#88#*#*'. \nIf neither of these work, this method will NOT work on your device.\n\n2. Once the service/test menu is open, do not touch anything on the screen. \nPlug one end of a USB cable into your computer, and the other end into the device.\n\n3. When the cable is plugged in, you may now close the message box to continue.")
    show_messagebox_at(500, 200, "nPhoneKIT", "Steps to continue (PLEASE READ CAREFULLY, THEN CLOSE THE MESSAGE BOX.):\n\n1. Plug one end of a USB cable into your computer, and the other end into the device.\n\n2. When the cable is plugged in, press ALLOW ACCESS TO PHONE DATA, and you may now close the message box to continue.")

def adbMenu():
    show_messagebox_at(500, 200, "nPhoneKIT", "Steps to continue (PLEASE READ CAREFULLY, THEN CLOSE THE MESSAGE BOX.):\n\n1. Enable developer options by going into settings,\nabout phone, software information, and tap build number \n 7 times. \n\n2. Go back into settings, scroll to\n the bottom, open developer options, scroll down a bit, \n find USB Debugging, and turn it on. \n\n 3. Plug one end of a USB cable into your computer, and the other end into the device.\n\n4. When the cable is plugged in, you may now close the message box to continue. If you ever see a dialog after this, please always click ALLOW.")

# ================================================
#  Simple functions to eliminate repetitive tasks
# ================================================

def remove_first_x_lines(s, x):
    return '\n'.join(s.splitlines()[x:])

def remove_last_x_lines(s, x):
    return '\n'.join(s.splitlines()[:-x]) if x else s

def rt():
    os.system("sudo bash -c 'rm -f tmp_output.txt'")
    os.system("sudo bash -c 'rm -f tmp_output_adb.txt'")

def readOutput(type):
    if type == "AT":
        with open("tmp_output.txt", "r") as f:
            output = f.read()
    elif type == "ADB":
        with open("tmp_output_adb.txt", "r") as f:
            output = f.read()
    return output

def show_messagebox_at(x, y, title, content):
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

def modemUnlock(manufacturer, softUnlock=False):
    if not enable_preload:
        if manufacturer == "SAMSUNG":
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

def testAT(MTPinstruction=False, text=f"Testing USB access (ETA: {ETA[3]})..."):
    print(text, end="")
    if MTPinstruction:
        MTPmenu()
    AT.send("AT")
    output = readOutput("AT")
    if "ok" in output.lower():
        print("  OK")
        rt()
        return True
    else:
        time.sleep(0.5)
        AT.send("AT")
        with open("tmp_output.txt", "r") as f:
            output = f.read()
        if "ok" in output.lower():
            print("  OK")
            rt()
            return True
        else:
            print("  FAIL")
            return False

# =============================================
#  Unlocking methods for different devices
# =============================================

def frp_unlock_pre_aug2022():
    ATcommands = [
        "AT+DUMPCTRL=1,0",
        "AT+DEBUGLVC=0,5",
        "AT+SWATD=0", # Removes some kind of proprietary SAMSUNG modem lock
        "AT+ACTIVATE=0,0,0", # So that you can ACTIVATE
        "AT+SWATD=1", # Then relocks it.
        "AT+DEBUGLVC=0,5"
    ]

    ADBcommands = [
        "shell settings put global setup_wizard_has_run 1",
        "shell settings put secure user_setup_complete 1",
        "shell content insert --uri content://settings/secure --bind name:s:DEVICE_PROVISIONED --bind value:i:1",
        "shell content insert --uri content://settings/secure --bind name:s:user_setup_complete --bind value:i:1",
        "shell content insert --uri content://settings/secure --bind name:s:INSTALL_NON_MARKET_APPS --bind value:i:1",
        "shell am start -c android.intent.category.HOME -a android.intent.action.MAIN"
    ]

    show_messagebox_at(500, 200, "nPhoneKIT", "Warning: This FRP unlock method may not always work. It will perform best on devices pre-2022. \n\n\n⚠️ This feature is intended only for owners of devices they personally own or are legally authorized to access. \nUnauthorized use may violate local laws. The developer assumes no responsibility for misuse.")

    print(f"Attempting to enable ADB access...", end="")

    show_messagebox_at(500, 200, "nPhoneKIT", "Steps to FRP Unlock (method A) (PLEASE READ CAREFULLY, THEN CLOSE THE MESSAGE BOX.):\n\n1. Factory reset the device using the recovery menu. \nIf this has already been done, proceed to step 2.\n\n2. Boot up the phone normally. You should see the setup screen. \nLocate a button called something like 'Emergency Call', and click it.\n This usually opens the dialer. If it calls immediately, \nthis method will NOT work on your device.\n\n3. After pressing the Emergency Call button, the dialer should be visible. \nPlease continue by typing in the following (without quotes): '*#0*#', and press CALL.\n A service/test menu should be opened. If not, you can also try '*#*#88#*#*'. \nIf neither of these work, this method will NOT work on your device.\n\n4. Once the service/test menu is open, do not touch anything on the screen. \nPlug one end of a USB cable into your computer, and the other end into the device.\n\n5. When the cable is plugged in, you may now close the message box to continue.")

    for command in ATcommands:
        AT.send(command)

    output = readOutput("AT")

    if "error" in output.lower():
        print("  FAIL")
        print("\nThis FRP unlock method will not work on your device.")
    else:
        print("  OK")
        print(f"Running Unlock...", end="")
        show_messagebox_at(500, 200, "nPhoneKIT", "Please make sure the USB Debugging prompt has appeared on your phone,\n and click ALLOW. If it does not appear, \n try unplugging and replugging the cable. \n If it still does not appear, this unlock is NOT compatible.")
        for command in ADBcommands:
            ADB.send(command)
        print("  OK")
        print("\nUNLOCK should be successful! To complete the unlock, please go into settings and perform a factory reset normally!")

def frp_unlock_aug2022_to_dec2022():
    commands = ['AT+SWATD=0', 'AT+ACTIVATE=0,0,0', 'AT+DEVCONINFO','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0', 'AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5','AT+SWATD=0','AT+ACTIVATE=0,0,0','AT+SWATD=1','AT+DEBUGLVC=0,5','AT+KSTRINGB=0,3','AT+DUMPCTRL=1,0','AT+DEBUGLVC=0,5']
    # These commands are supposed to overwhelm the phone and trick it into enabling ADB. The rest after this is the same as the other unlock method.

    ADBcommands = [
        "shell settings put global setup_wizard_has_run 1",
        "shell settings put secure user_setup_complete 1",
        "shell content insert --uri content://settings/secure --bind name:s:DEVICE_PROVISIONED --bind value:i:1",
        "shell content insert --uri content://settings/secure --bind name:s:user_setup_complete --bind value:i:1",
        "shell content insert --uri content://settings/secure --bind name:s:INSTALL_NON_MARKET_APPS --bind value:i:1",
        "shell am start -c android.intent.category.HOME -a android.intent.action.MAIN"
    ]

    show_messagebox_at(500, 200, "nPhoneKIT", "Warning: This FRP unlock method may not always work. It will perform best on most devices mid-2022. \n\n\n⚠️ This feature is intended only for owners of devices they personally own or are legally authorized to access. \nUnauthorized use may violate local laws. The developer assumes no responsibility for misuse.")

    print(f"Attempting to enable ADB access...", end="")

    show_messagebox_at(500, 200, "nPhoneKIT", "Steps to FRP Unlock (PLEASE READ CAREFULLY, THEN CLOSE THE MESSAGE BOX.):\n\n1. Factory reset the device using the recovery menu. \nIf this has already been done, proceed to step 2.\n\n2. Boot up the phone normally. You should see the setup screen. \nLocate a button called something like 'Emergency Call', and click it.\n This usually opens the dialer. If it calls immediately, \nthis method will NOT work on your device.\n\n3. After pressing the Emergency Call button, the dialer should be visible. \nPlease continue by typing in the following (without quotes): '*#0*#', and press CALL.\n A service/test menu should be opened. If not, you can also try '*#*#88#*#*'. \nIf neither of these work, this method will NOT work on your device.\n\n4. Once the service/test menu is open, do not touch anything on the screen. \nPlug one end of a USB cable into your computer, and the other end into the device.\n\n5. When the cable is plugged in, you may now close the message box to continue.")

    for command in commands:
        AT.send(command)

    output = readOutput("AT")

    if "error" in output.lower():
        print("  FAIL")
        print("\nThis FRP unlock method will not work on your device.")
    else:
        print("  OK")
        print(f"Running Unlock...", end="")
        show_messagebox_at(500, 200, "nPhoneKIT", "Please make sure the USB Debugging prompt has appeared on your phone,\n and click ALLOW. If it does not appear, \n try unplugging and replugging the cable. \n If it still does not appear, this unlock is NOT compatible.")
        for command in ADBcommands:
            ADB.send(command)
        print("  OK")
        print("\nUNLOCK should be successful! To complete the unlock, please go into settings and perform a factory reset normally!")
    
def frp_unlock_2024():
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

    ADBcommands = [
        "shell settings put global setup_wizard_has_run 1",
        "shell settings put secure user_setup_complete 1",
        "shell content insert --uri content://settings/secure --bind name:s:DEVICE_PROVISIONED --bind value:i:1",
        "shell content insert --uri content://settings/secure --bind name:s:user_setup_complete --bind value:i:1",
        "shell content insert --uri content://settings/secure --bind name:s:INSTALL_NON_MARKET_APPS --bind value:i:1",
        "shell am start -c android.intent.category.HOME -a android.intent.action.MAIN"
    ]

    show_messagebox_at(500, 200, "nPhoneKIT", "Warning: This FRP unlock method may not always work. It will perform best on most devices. \n\n\n⚠️ This feature is intended only for owners of devices they personally own or are legally authorized to access. \nUnauthorized use may violate local laws. The developer assumes no responsibility for misuse.")

    print(f"Attempting to enable ADB access...", end="")

    show_messagebox_at(500, 200, "nPhoneKIT", "Steps to FRP Unlock (method A) (PLEASE READ CAREFULLY, THEN CLOSE THE MESSAGE BOX.):\n\n1. Factory reset the device using the recovery menu. \nIf this has already been done, proceed to step 2.\n\n2. Boot up the phone normally. You should see the setup screen. \nLocate a button called something like 'Emergency Call', and click it.\n This usually opens the dialer. If it calls immediately, \nthis method will NOT work on your device.\n\n3. After pressing the Emergency Call button, the dialer should be visible. \nPlease continue by typing in the following (without quotes): '*#0*#', and press CALL.\n A service/test menu should be opened. If not, you can also try '*#*#88#*#*'. \nIf neither of these work, this method will NOT work on your device.\n\n4. Once the service/test menu is open, do not touch anything on the screen. \nPlug one end of a USB cable into your computer, and the other end into the device.\n\n5. When the cable is plugged in, you may now close the message box to continue.")

    for command in commands:
        AT.send(command)

    output = readOutput("AT")

    if "error" in output.lower():
        print("  FAIL")
        print("\nThis FRP unlock method will not work on your device.")
    else:
        print("  OK")
        print(f"Running Unlock...", end="")
        show_messagebox_at(500, 200, "nPhoneKIT", "Please make sure the USB Debugging prompt has appeared on your phone,\n and click ALLOW. If it does not appear, \n try unplugging and replugging the cable. \n If it still does not appear, this unlock is NOT compatible.")
        for command in ADBcommands:
            ADB.send(command)
        print("  OK")
        print("\nUNLOCK should be successful! To complete the unlock, please go into settings and perform a factory reset normally!")


def general_frp_unlock():
    info = verinfo(False)
    if "Model: SM" in info:
        frp_unlock_pre_aug2022()
    else:
        # to do, add FULLY universal FRP unlock
        print("Your device is not supported.")

def LG_screen_unlock():
    show_messagebox_at(500, 200, "nPhoneKIT", "🔓 LG Screen Unlock\n\nThe LG Screen Unlock will simply unlock the phone's \nscreen, without erasing data whatsoever.\n\nIt is only supported on these LG phones. \nPlease UNPLUG all devices and CLOSE this window if you do not have one of the below devices:\n\nLG G4 H815\nLG G4 H811\nLG G4 VS986\nLG V10 H901\nLG V10 H960\nLG G3 D855\nLG G3 D851\nLG Stylo 2 LS775\nLG Tribute HD LS676\nLG Phoenix 2 K371\nLG Aristo M210\nLG Leon H345\n\nIf one of these devices is yours, you may click OK and follow the instructions.")
    print(f"Running Screen Unlock Command...", end="")
    # Prepare phone for unlock
    show_messagebox_at(600, 100, "nPhoneKIT", "🔓 LG Screen Unlock\n\nPlease plug your LG phone into your computer, \nthen press OK. \n\n(Note: For this step, you will need GCC \n installed on your machine. If it's not installed, \nplease install it now, then click OK once your phone is plugged in.)")
    
    time.sleep(1)
    if AT.usbswitch("-l", "LG Screen Unlock"):
        rt()
        AT.send('AT%KEYLOCK=0') # This AT command SHOULD unlock the screen instantly.
        with open("tmp_output.txt", "r") as f:
            output = f.read()
        # debug only: print("\n\nOutput: \n\n" + output + "\n\n")
        if "error" in output or "Error" in output:
            print("  FAIL\n")
            print("There was an error in unlocking the screen. Please open a GitHub issue with your phone model, and the contents of tmp_output.txt which should be in the same directory as main.py.")
        else:
            rt()
            print("  OK\n")
            print("Screen should be unlocked. If it's not, please open a GitHub issue with your phone model, and exactly what you did.")

# ==============================================
#  Simple functions that do stuff to the device
# ==============================================

def verinfo(gui=True):
    if gui:
        if enable_preload: # Skip all the nonsense and cut straight to the action, no "testAT" nonsense. We're prioritizing speed.
            print("Getting version info...")
            AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
            output = readOutput("AT") # Output is retrieved from the command
            output = parse_devconinfo(output) # Make the output actually readable
            print(output) # Print the version info to the output box
        else: 
            if testAT(True, text=f"Getting version info..."): # We should verify AT is working before running the below code
                if not enable_preload:
                    modemUnlock("SAMSUNG") # Run the command to allow more AT access for SAMSUNG devices unless preloading is enabled
                    rt() # Flush the command output file
                AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
                output = readOutput("AT") # Output is retrieved from the command
                output = parse_devconinfo(output) # Make the output actually readable
                print(output) # Print the version info to the output box
    else:
        if testAT(True, text=f"Getting version info..."): # We should verify AT is working before running the below code
            if not enable_preload:
                modemUnlock("SAMSUNG") # Run the command to allow more AT access for SAMSUNG devices unless preloading is enabled
                rt() # Flush the command output file
            AT.send("AT+DEVCONINFO") # Only works when the modem is working with modemUnlock("SAMSUNG")
            output = readOutput("AT") # Output is retrieved from the command
            output = parse_devconinfo(output) # Make the output actually readable (parse the output)
            return output # Return the version info

def wifitest():
    success = [
    "AT+WIFITEST=9,9,9,1",
    "+WIFITEST:9,",
    "OK"
    ]

    print(f"Opening WIFITEST...", end="")
    MTPmenu()
    modemUnlock("SAMSUNG")
    AT.send("AT+SWATD=1")
    rt()
    AT.send("AT+WIFITEST=9,9,9,1")
    output = readOutput("AT")
    counter = 0
    for i in success:
        if i in output:
            counter += 1
    if counter == 3:
        print("  OK")
    else:
        print("  FAIL")

def reboot():
    print(f"Crashing phone to reboot...", end="")
    MTPmenu()
    rt()
    try:
        AT.send("AT+CFUN=1,1") # Crashes the phone immediately.
    except Exception as e:
        if "disconnected" in str(e):
            print("  OK") # Error opening serial means that the command worked, because it reset the phone before it could give a response.
    output = readOutput("AT")
    if "OK" in output:
        print("  FAIL")
        print("\nThe phone did not seem to crash. (If this is a Samsung phone, you must click the reboot option in the SAMSUNG tab on the left.)")

def reboot_sam():
    print(f"Crashing phone to reboot...", end="")
    MTPmenu()
    modemUnlock("SAMSUNG", True)
    rt()
    try:
        AT.send("AT+CFUN=1,1") # Crashes the phone immediately.
    except Exception as e:
        if "disconnected" in str(e):
            print("  OK") # Error opening serial means that the command worked, because it reset the phone before it could give a response.
    output = readOutput("AT")
    if "OK" in output:
        print("  FAIL")
        print("\nThe phone did not seem to crash. (If this is a Samsung phone, you must click the reboot option in the SAMSUNG tab on the left.)")

def bloatRemove():
    adbMenu()
    # UNFINISHED

def reboot_download_sam():
    print("Rebooting to Download Mode...", end="")
    MTPmenu() 
    AT.send("AT+FUS?") # Thankfully, no modem unlocking required for this command.
    print(" OK")

# ===================================
#  Tkinter GUI Stuff
# ===================================

class RedirectText:
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

current_brand = "Android"  
brand_buttons = {}  
brand_frames = {} 

def select_brand(name):
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

def main():
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

    for name in ["Samsung", "LG", "Android"]:
        btn = tk.Button(button_bar, text=name, font=("Helvetica", 20), width=15,
                        command=lambda n=name: select_brand(n), bg=background_color_2)
        btn.pack(side="left", padx=10, pady=10)
        brand_buttons[name] = btn

    content_area = tk.Frame(left_frame, bg=background_color_1, bd=0, relief="flat")
    content_area.pack(fill="both", expand=True)

    samsung_frame_A = tk.Frame(content_area, bg=background_color_1, bd=0, relief="flat")
    lg_frame = tk.Frame(content_area, bg=background_color_1, bd=0, relief="flat")
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
        tk.Label(top_outputbar, text="nPhoneKIT", bg="#9191eb", fg="black", font=("Helvetica", 24, "bold")).pack(pady=10)
    else:
        tk.Label(top_outputbar, text="nPhoneKIT", bg="#6969f1", fg="white", font=("Helvetica", 24, "bold")).pack(pady=10)


    brand_frames["Samsung"] = samsung_frame_A
    brand_frames["LG"] = lg_frame
    brand_frames["Android"] = general_frame

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

    def animate_window_open(target_width=1550, speed=anim_speed, delay=1):
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
    add_button(samsung_frame_A, "FRP Unlock \n(Security Patch 𝟤𝟢𝟤𝟤)\n Works on most devices! 🔓", frp_unlock_2024, "Attempts to unlock a phone \nwhich has been FRP locked. \n\n Usually works on most devices,\nincluding some devices with \nSecurity Patch Level 𝟤𝟢𝟤𝟦.", emoji_font_bold)
    add_button(samsung_frame_A, "FRP Unlock \n(Security Patch August-𝟤𝟢𝟤𝟤 \nthrough December-𝟤𝟢𝟤𝟤) 🔓", frp_unlock_pre_aug2022, "Attempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically from Security\n Patch Level August 𝟤𝟢𝟤𝟤 through\n the December 𝟤𝟢𝟤𝟤 patch.", emoji_font_smaller)
    add_button(samsung_frame_A, "FRP Unlock \n(Security Patch Pre-August-𝟤𝟢𝟤𝟤) 🔓", frp_unlock_pre_aug2022, "Attempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically before Security\n Patch Level August 𝟤𝟢𝟤𝟤", emoji_font_smaller)
    add_button(samsung_frame_A, "Get Version Info 📝", verinfo, "Gets firmware version info from most SAMSUNG phones. \n\n(Works on S24 Series)")
    add_button(samsung_frame_A, "   Reboot 🔄   ", reboot_sam, "Crashes the phone using AT commands as an\nalternate reboot method.")
    add_button(samsung_frame_A, "Reboot to Download Mode", reboot_download_sam, "Reboots into Download Mode easily.")
    add_button(samsung_frame_A, "  WIFITEST 🛜  ", wifitest, "Opens a hidden WIFITEST app, even \nif the setup wizard hasn't completed. \n\n(Works on S24 Series)")


    add_button(lg_frame, "Screen Unlock 🔓", LG_screen_unlock,"Unlock the screen of an LG phone, without losing data.\n\nOnly click if you are using one of the following phones:\n\nLG G4 H815\nLG G4 H811\nLG G4 VS986\nLG V10 H901\nLG V10 H960\nLG G3 D855\nLG G3 D851\nLG Stylo 2 LS775\nLG Tribute HD LS676\nLG Phoenix 2 K371\nLG Aristo M210\nLG Leon H345")
    

    add_button(general_frame, "Universal FRP Unlock 🔓", general_frp_unlock, "This is a universal FRP unlock method. It \nshould work on most manufacturers. \n\n=======================================\n\nAttempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically =< Android 8")
    add_button(general_frame, "   Reboot 🔄   ", reboot, "Crashes the phone using AT commands as an\nalternate reboot method.")
    add_button(general_frame, "Remove Carrier Bloatware", bloatRemove, "")

    # Select default brand
    select_brand(current_brand)

    print(f"Welcome to nPhoneKIT v{version}")
    print(f"\n\nNew in v1.2.1: Made all AT commands 10-20x faster by including pySerial in main.py")
    print(f"\n\nNew in v1.2.0: Confirmed WIFITEST and VER_INFO support for S24 Series\n\n")

    animate_window_open()

    root.mainloop()

def main_old():
    root = tk.Tk()
    root.title("nPhoneKIT")
    root.geometry("1550x800")
    root.tk.call('tk', 'scaling', 3)

    # Panels setup
    left_frame = tk.Frame(root, width=1050, bg="#dddddd")
    left_frame.pack(side="left", fill="y")
    samsung_frame_A = tk.Frame(left_frame, width=525, bg="#dddddd")
    samsung_frame_A.pack(side="left", fill="y", padx=20, pady=(20, 0))
    lg_frame = tk.Frame(left_frame, width=525, bg="#dddddd")
    lg_frame.pack(side="left", fill="y", padx=20, pady=(20, 0))
    general_frame = tk.Frame(left_frame, width=525, bg="#dddddd")
    general_frame.pack(side="left", fill="y", padx=20, pady=(20, 0))

    # Output panel
    right_frame = tk.Frame(root, width=200)
    right_frame.pack(side="right", fill="both")
    top_outputbar = tk.Frame(right_frame, bg="#6969f1", height=100)
    top_outputbar.pack(fill="x")
    output_box = tk.Text(right_frame, width=525, wrap="word", bg="#f5f5f5", font=("Courier", 15))
    output_box.pack(expand=True, fill="both")
    output_box.tag_configure("fail", foreground="red")
    output_box.tag_configure("ok",  foreground="green")
    sys.stdout = RedirectText(output_box)
    sys.stderr = RedirectText(output_box)

    # Header
    tk.Label(top_outputbar, text="nPhoneKIT", bg="#6969f1", fg="white", font=("Helvetica", 24, "bold")).pack(pady=10)

    tk.Label(samsung_frame_A, text="SAMSUNG", fg="black", font=("Helvetica", 30, "bold")).pack(pady=10)
    tk.Label(lg_frame, text="LG", fg="red", font=("Helvetica", 30, "bold")).pack(pady=10)
    tk.Label(general_frame, text="Android", fg="green", font=("Helvetica", 30, "bold")).pack(pady=10)

    emoji_font_bold = font.Font(family="Noto Color Emoji", size=10, weight="bold")
    emoji_font = font.Font(family="Noto Color Emoji", size=10)
    emoji_font_smaller = font.Font(family="Noto Color Emoji", size=8)

    # Tooltip label (hidden by default)
    tooltip = tk.Label(root, text="", bg="yellow", font=("Helvetica", 12), bd=1, relief="solid")
    tooltip.place_forget()

    def show_tooltip(event, text):
        tooltip.config(text=text)
        x = event.x_root - root.winfo_rootx() + 10
        y = event.y_root - root.winfo_rooty() + 10
        tooltip.place(x=x, y=y)
        tooltip.lift()

    def hide_tooltip(event):
        tooltip.place_forget()

    def add_button(frame, text, cmd, prefix, fontType=emoji_font):
        btn = tk.Button(frame, text=text, command=cmd, font=fontType)
        btn.pack(pady=10)
        btn.bind("<Enter>", lambda e, t=text: show_tooltip(e, f"{prefix}"))
        btn.bind("<Leave>", hide_tooltip)
    
    add_button(samsung_frame_A, "FRP Unlock \n(Security Patch 2024)\n Works on most devices! 🔓", frp_unlock_2024, "Attempts to unlock a phone \nwhich has been FRP locked. \n\n Usually works on most devices,\nincluding some devices with \nSecurity Patch Level 2024.", emoji_font_bold)
    add_button(samsung_frame_A, "FRP Unlock \n(Security Patch August-2022 \nthrough December-2022) 🔓", frp_unlock_pre_aug2022, "Attempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically from Security\n Patch Level August 2022 through\n the December 2022 patch.", emoji_font_smaller)
    add_button(samsung_frame_A, "FRP Unlock \n(Security Patch Pre-August-2022) 🔓", frp_unlock_pre_aug2022, "Attempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically before Security\n Patch Level August 2022", emoji_font_smaller)
    add_button(samsung_frame_A, "Get Version Info 📝", verinfo, "Gets firmware version info from most SAMSUNG phones. \n\n(Works on S24 Series)")
    add_button(samsung_frame_A, "   Reboot 🔄   ", reboot_sam, "Crashes the phone using AT commands as an\nalternate reboot method.")
    add_button(samsung_frame_A, "Reboot to Download Mode", reboot_download_sam, "Reboots into Download Mode easily.")
    add_button(samsung_frame_A, "  WIFITEST 🛜  ", wifitest, "Opens a hidden WIFITEST app, even \nif the setup wizard hasn't completed. \n\n(Works on S24 Series)")


    add_button(lg_frame, "Screen Unlock 🔓", LG_screen_unlock,"Unlock the screen of an LG phone, without losing data.\n\nOnly click if you are using one of the following phones:\n\nLG G4 H815\nLG G4 H811\nLG G4 VS986\nLG V10 H901\nLG V10 H960\nLG G3 D855\nLG G3 D851\nLG Stylo 2 LS775\nLG Tribute HD LS676\nLG Phoenix 2 K371\nLG Aristo M210\nLG Leon H345")
    

    #add_button(general_frame, "Universal FRP Unlock 🔓", general_frp_unlock, "This is a universal FRP unlock method. It \nshould work on most manufacturers. \n\n=======================================\n\nAttempts to unlock a phone \nwhich has been FRP locked. \n\n Usually only works on older devices,\nspecifically =< Android 8")
    add_button(general_frame, "   Reboot 🔄   ", reboot, "Crashes the phone using AT commands as an\nalternate reboot method.")
    root.mainloop()

def is_root():
    return os.geteuid() == 0  # Only works on Unix/Linux/macOS

if not is_root():
    root = tk.Tk()
    root.withdraw()

    messagebox.showwarning("nPhoneKIT", "Please close this window and rerun using SUDO (or Run as Administrator on Windows). Most features/tools will not work otherwise.")
    sys.exit(1)

if update_check:
    check_for_update()

serman1 = SerialManager()

def preload_thread():
    asyncio.run(preload_samsung_modem(serman1))

threading.Thread(target=preload_thread, daemon=True).start()

if __name__ == "__main__":
    rt()
    main()
