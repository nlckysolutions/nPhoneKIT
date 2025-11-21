#
#             ███████████  █████                                   █████   ████ █████ ███████████
#            ░░███░░░░░███░░███                                   ░░███   ███░ ░░███ ░█░░░███░░░█
#  ████████   ░███    ░███ ░███████    ██████  ████████    ██████  ░███  ███    ░███ ░   ░███  ░ 
# ░░███░░███  ░██████████  ░███░░███  ███░░███░░███░░███  ███░░███ ░███████     ░███     ░███    
#  ░███ ░███  ░███░░░░░░   ░███ ░███ ░███ ░███ ░███ ░███ ░███████  ░███░░███    ░███     ░███    
#  ░███ ░███  ░███         ░███ ░███ ░███ ░███ ░███ ░███ ░███░░░   ░███ ░░███   ░███     ░███    
#  ████ █████ █████        ████ █████░░██████  ████ █████░░██████  █████ ░░████ █████    █████   
# ░░░░ ░░░░░ ░░░░░        ░░░░ ░░░░░  ░░░░░░  ░░░░ ░░░░░  ░░░░░░  ░░░░░   ░░░░ ░░░░░    ░░░░░    
#

# IMPORTS AND WHY EACH ONE IS NEEDED

import time # Waiting before executing something
import os # Executing most commands
import tkinter as tk # Main GUI (deprecated, slowly being removed)
from tkinter import ttk # Styling for GUI (deprecated)
from tkinter import messagebox # Opening message/warning boxes
from tkinter import font # Customizing GUI font
from pathlib import Path # Importing settings
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
from PyQt5.QtWidgets import (
    QApplication, QWidget, QLabel, QVBoxLayout, QHBoxLayout, QPushButton, QListWidget, QListWidgetItem
)
from PyQt5.QtCore import Qt, QTimer, QPointF
from PyQt5.QtGui import QPainter, QPen, QFont
from datetime import datetime, timedelta
from functools import partial # Register button clicks to functions
import shutil # Fastboot partition eraser for Motorola

## nPhoneKIT permissions (these are the things that nPhoneKIT is capable of doing):

# Communicate with USB devices using ADB, MTP, and AT commands.
# Communicate with external servers to verify whether an action worked or not.
# Open a new tab in the default browser
# Checking and getting basic information about the current system

version = "1.5.0"

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

debugMode = False # If True, removes root/admin requirement. Most serial/usb features will stop working. As of v1.5.0, this will only affect Windows users.

SETTINGS_PATH = Path("settings.json") # Load settings externally

firstunlock = False # This variable helps ModemPreload work

default_settings = {
    "dark_theme": True,
    "hacker_font": False,
    "slower_animations": False,
    "update_check": True,
    "impatient": False,
    "enable_preload": True,
    "debug_info": False,
    "i_know_what_im_doing": False,
    "basic_success_checks": True
}

if SETTINGS_PATH.exists(): # If settings, load, otherwise use default settings.
    with open(SETTINGS_PATH, "r") as f:
        settings = json.load(f)
else:
    settings = default_settings.copy()
    with open(SETTINGS_PATH, "w") as f:
        json.dump(settings, f, indent=2)

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
strings = load_strings("strings.xml") # Load almost every string from strings.xml (ez translations)

# Load settings
def load_settings():
    with open(SETTINGS_PATH, "r") as f:
        return json.load(f)

# Save settings
def save_settings(new_settings):
    with open(SETTINGS_PATH, "w") as f:
        json.dump(new_settings, f, indent=2)

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

def check_serial_permissions():
    if os_config == "LINUX":
        import grp
        import getpass
        import platform

        user = getpass.getuser()

        # Serial device groups used across most distros
        serial_groups = ["dialout", "uucp", "lock", "tty"]

        # Collect groups the user is currently in
        user_groups = [g.gr_name for g in grp.getgrall() if user in g.gr_mem]

        # Also check primary group ID (some distros put uucp as primary)
        try:
            primary_group = grp.getgrgid(os.getgid()).gr_name
            user_groups.append(primary_group)
        except:
            pass

        # Check if user is good
        for g in serial_groups:
            if g in user_groups:
                return True  # Permissions OK

        # If we reach here, user is missing required groups
        # Decide which command to show based on distro
        distro = platform.system()

        if distro == "Linux":
            # Try reading OS-release for better accuracy
            import distro as distro_lib
            name = distro_lib.id()

            if name in ["ubuntu", "debian", "linuxmint", "zorin"]:
                cmd = f"sudo usermod -aG dialout {user}"
            elif name in ["arch", "endeavouros", "cachyos", "manjaro", "garuda"]:
                cmd = f"sudo usermod -aG uucp,lock {user}"
            elif name in ["fedora", "rhel", "centos"]:
                cmd = f"sudo usermod -aG dialout {user}"
            else:
                # Fallback universal command
                cmd = f"sudo usermod -aG dialout,uucp,lock {user}"
        else:
            cmd = "Unsupported OS for serial permissions."

        # Show Tkinter window with copy-paste command
        root = tk.Tk()
        root.title("Serial Permission Fix Required")
        root.geometry("500x250")

        label = tk.Label(root, text="To enable serial access, run this command in your terminal:", font=("Arial", 12))
        label.pack(pady=10)

        text_box = tk.Text(root, height=2, font=("Courier", 12))
        text_box.pack(padx=20, pady=10, fill="both")
        text_box.insert("1.0", cmd)

        # Make text read-only
        text_box.config(state="disabled")

        reboot_label = tk.Label(root, text="After running the command, reboot your system.", font=("Arial", 10))
        reboot_label.pack(pady=10)

        ok_button = tk.Button(root, text="OK", command=root.destroy)
        ok_button.pack(pady=10)

        root.mainloop()

        return False
    else:
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

import tkinter as tk
from tkinter import ttk, font
from datetime import datetime, timedelta
import math
import threading
import multiprocessing
from typing import List, Optional, Tuple

# Helper: worker used when we need to run the dialog in a new process.
# This must be a top-level function for multiprocessing to work reliably.
def _stw_worker(conn, title, desc, pros, cons, minutes, execute_text, cancel_text, win_w, win_h):
    """
    Runs in child process: constructs a Tk window, shows it, sends result (True/False)
    back through the connection, and exits.
    """
    try:
        root = tk.Tk()
        #root.withdraw()  # we'll show our Toplevel dialog
    except Exception:
        # If tk can't initialize, return False
        try:
            conn.send(False)
        except Exception:
            pass
        conn.close()
        return

    # Local stw implementation for child (almost same as parent inline version)
    # Colors & sizes
    bg = "#F5F7FB"
    card_bg = "#FFFFFF"
    text_primary = "#0F172A"
    text_secondary = "#374151"
    accent1 = "#2563EB"
    accent2 = "#0EA5E9"
    cons_red = "#B45309"
    pad = 14

    # Fonts (best-effort)
    try:
        title_font = font.nametofont("TkHeadingFont").copy()
        title_font.configure(size=18, weight="bold", family="Segoe UI")
    except Exception:
        title_font = ("Segoe UI", 18, "bold")
    desc_font = ("Segoe UI", 12)
    list_font = ("Segoe UI", 12)
    emoji_font = ("Noto Color Emoji", 14)

    # result container
    result = {"value": False}

    # build dialog
    win = root
    win.title("nPhoneKIT")
    win.geometry(f"{win_w}x{win_h}")
    win.configure(bg=bg)
    win.resizable(False, False)

    # When closed without pressing buttons
    def _on_close():
        result["value"] = False
        try:
            win.quit()
        except Exception:
            pass

    win.protocol("WM_DELETE_WINDOW", _on_close)

    # layout
    content = tk.Frame(win, bg=bg)
    content.place(relx=0, rely=0, relwidth=1, relheight=1)

    left_w = int((win_w - pad*3) * 0.66)
    right_w = (win_w - pad*3) - left_w

    left_frame_outer = tk.Frame(content, bg=bg)
    left_frame_outer.place(x=pad, y=pad, width=left_w, height=win_h - pad*4 - 60)

    right_frame = tk.Frame(content, bg=bg)
    right_frame.place(x=pad + left_w + pad, y=pad, width=right_w, height=win_h - pad*4 - 60)

    # Scrollable left card
    canvas = tk.Canvas(left_frame_outer, borderwidth=0, highlightthickness=0, bg=bg)
    vscroll = ttk.Scrollbar(left_frame_outer, orient="vertical", command=canvas.yview)
    canvas.configure(yscrollcommand=vscroll.set)
    vscroll.pack(side="right", fill="y")
    canvas.pack(side="left", fill="both", expand=True)
    left_card = tk.Frame(canvas, bg=card_bg)
    canvas.create_window((0,0), window=left_card, anchor="nw")

    def _on_configure(e):
        canvas.configure(scrollregion=canvas.bbox("all"))
    left_card.bind("<Configure>", _on_configure)

    # --- turn OFF scrolling (quick toggle) ---
    try:
        # hide the scrollbar widget so it isn't visible
        vscroll.pack_forget()
    except Exception:
        pass

    # stop the canvas from driving the scrollbar
    try:
        canvas.configure(yscrollcommand=lambda *args: None)
    except Exception:
        pass

    # stop adjusting scrollregion when left_card resizes (disable the handler if present)
    # if you previously bound left_card to update scrollregion, replace it with a no-op:
    try:
        left_card.unbind("<Configure>")
    except Exception:
        pass

    # disable mousewheel scrolling that may have been bound globally
    try:
        canvas.unbind_all("<MouseWheel>")
        canvas.unbind_all("<Button-4>")
        canvas.unbind_all("<Button-5>")
    except Exception:
        pass

    # optionally make the canvas non-expand so it won't try to scroll content
    try:
        canvas.pack_configure(expand=False, fill="both")
    except Exception:
        pass

    # Title
    lbl_title = tk.Label(left_card, text=title, font=title_font, bg=card_bg, fg=text_primary, wraplength=left_w-40, justify="left")
    lbl_title.pack(anchor="w", pady=(6,4))

    # Description (wrapped)
    desc_lbl = tk.Label(left_card, text=desc, font=desc_font, bg=card_bg, fg=text_secondary, wraplength=left_w-40, justify="left")
    desc_lbl.pack(anchor="w", pady=(0,8))

    # Pros
    pros_hdr = tk.Label(left_card, text="Pros", font=("Segoe UI", 13, "bold"), bg=card_bg, fg=text_primary)
    pros_hdr.pack(anchor="w", pady=(6,2))
    if not pros:
        pros = []
    for p in pros:
        row = tk.Frame(left_card, bg=card_bg)
        row.pack(fill="x", anchor="w", pady=2)
        em = tk.Label(row, text="✅", font=emoji_font, bg=card_bg)
        em.pack(side="left", anchor="n")
        txt = tk.Label(row, text=str(p), font=list_font, bg=card_bg, fg="#0B1720", wraplength=left_w-80, justify="left", anchor="w")
        txt.pack(side="left", anchor="w", padx=(8,0))

    # Cons
    cons_hdr = tk.Label(left_card, text="Cons", font=("Segoe UI", 13, "bold"), bg=card_bg, fg=text_primary)
    cons_hdr.pack(anchor="w", pady=(10,2))
    if not cons:
        cons = []
    for c in cons:
        row = tk.Frame(left_card, bg=card_bg)
        row.pack(fill="x", anchor="w", pady=2)
        em = tk.Label(row, text="❌", font=emoji_font, bg=card_bg)
        em.pack(side="left", anchor="n")
        txt = tk.Label(row, text=str(c), font=list_font, bg=card_bg, fg=cons_red, wraplength=left_w-80, justify="left")
        txt.pack(side="left", anchor="w", padx=(8,0))

    left_card.update_idletasks()
    canvas.configure(scrollregion=canvas.bbox("all"))
    canvas.bind_all("<MouseWheel>", lambda e: canvas.yview_scroll(int(-1*(e.delta/120)), "units"), add="+")

    # Buttons area
    btn_area = tk.Frame(content, bg=bg)
    btn_area.place(x=pad, y=win_h - pad - 56, width=win_w - pad*2, height=56)

    def _on_cancel():
        result["value"] = False
        win.quit()

    cancel_btn = tk.Button(btn_area, text=cancel_text, command=_on_cancel,
                           bg=card_bg, fg=text_secondary, bd=1, relief="solid", padx=12, pady=6)
    cancel_btn.place(x=pad, y=6, width=120, height=44)

    # execute button canvas (gradient)
    exec_canvas = tk.Canvas(btn_area, bd=0, highlightthickness=0)
    exec_canvas.place(x=win_w - pad - 160, y=6, width=160, height=44)

    # draw gradient approximation
    def _draw_gradient(cnv, x0, y0, x1, y1, color1, color2, steps=48):
        def _hex_to_rgb(h):
            h = h.lstrip("#")
            return tuple(int(h[i:i+2], 16) for i in (0,2,4))
        def _rgb_to_hex(rgb):
            return "#{:02x}{:02x}{:02x}".format(*rgb)
        c1 = _hex_to_rgb(color1)
        c2 = _hex_to_rgb(color2)
        width = x1 - x0
        for i in range(steps):
            t = i / steps
            r = int(c1[0] + (c2[0]-c1[0]) * t)
            g = int(c1[1] + (c2[1]-c1[1]) * t)
            b = int(c1[2] + (c2[2]-c1[2]) * t)
            cnv.create_rectangle(x0 + i*(width/steps), y0, x0 + (i+1)*(width/steps), y1, outline="", fill=_rgb_to_hex((r,g,b)))
    _draw_gradient(exec_canvas, 0, 0, 160, 44, accent1, accent2, steps=64)
    exec_canvas.create_text(80, 22, text=execute_text, fill="white", font=("Segoe UI", 11, "bold"))

    def _on_execute(event=None):
        result["value"] = True
        win.quit()

    exec_canvas.bind("<Button-1>", _on_execute)
    exec_canvas.bind("<Return>", _on_execute)

    # Right column: clock canvas
    clock_w = right_w - 24
    clock_h = (win_h - pad*4 - 60)//2
    clock_canvas = tk.Canvas(right_frame, width=clock_w, height=clock_h, bg=card_bg, bd=0, highlightthickness=0)
    clock_canvas.pack(pady=(0,8))

    def _draw_clock():
        clock_canvas.delete("all")
        w = max(1, clock_canvas.winfo_width())
        h = max(1, clock_canvas.winfo_height())
        cx = w//2
        cy = h//2
        radius = int(min(w,h)*0.42)

        clock_canvas.create_oval(cx-radius, cy-radius, cx+radius, cy+radius, fill="#FFFFFF", outline="#D1D5DB", width=2)
        for i in range(60):
            ang = math.radians(i * 6)
            outer_x = cx + radius * math.sin(ang)
            outer_y = cy - radius * math.cos(ang)
            inner_len = radius * (0.90 if (i % 5 == 0) else 0.96)
            inner_x = cx + inner_len * math.sin(ang)
            inner_y = cy - inner_len * math.cos(ang)
            clock_canvas.create_line(inner_x, inner_y, outer_x, outer_y, fill="#9CA3AF", width=1)

        now = datetime.now()
        end = now + timedelta(minutes=minutes)
        minute_now = now.minute + now.second/60.0
        minute_end = end.minute + end.second/60.0

        def to_tk_angle(deg_clockwise_from_12):
            return 90 - deg_clockwise_from_12

        span_clockwise = (minute_end - minute_now) % 60.0
        span_deg = span_clockwise * 6.0
        start_angle = to_tk_angle(minute_now)
        # tkinter create_arc accepts floats but better cast to ints for some backends
        try:
            clock_canvas.create_arc(cx-radius+8, cy-radius+8, cx+radius-8, cy+radius-8,
                                    start=start_angle, extent=-span_deg, fill="#93C5FD", outline="")
        except Exception:
            clock_canvas.create_arc(int(cx-radius+8), int(cy-radius+8), int(cx+radius-8), int(cy+radius-8),
                                    start=int(start_angle), extent=int(-span_deg), fill="#93C5FD", outline="")

        def draw_hand(angle_deg, length_factor, color, width=4):
            rad = math.radians(angle_deg)
            x = cx + length_factor * radius * math.sin(rad)
            y = cy - length_factor * radius * math.cos(rad)
            clock_canvas.create_line(cx, cy, x, y, fill=color, width=width, capstyle="round")

        draw_hand(minute_now*6.0, 0.78, "#2563EB", 4)
        draw_hand(minute_end*6.0, 0.78, "#F59E0B", 4)
        clock_canvas.create_oval(cx-4, cy-4, cx+4, cy+4, fill="#111827", outline="")

    # initial draw & periodic refresh
    win.update_idletasks()
    _draw_clock()
    def _tick():
        try:
            _draw_clock()
            win.after(1000, _tick)
        except tk.TclError:
            pass
    win.after(1000, _tick)

    eta_time = (datetime.now() + timedelta(minutes=minutes)).strftime("%I:%M %p").lstrip("0")
    eta_label = tk.Label(right_frame, text=f"⏱ {minutes} min\n🕓 Ends at: {eta_time}", bg=card_bg, fg=text_primary, font=("Segoe UI", 12), justify="center")
    eta_label.pack(pady=(6,0))

    # keyboard handling
    def _on_key(event):
        if event.keysym == "Return":
            _on_execute()
        elif event.keysym == "Escape":
            _on_cancel()
    win.bind_all("<Key>", _on_key)

    # center window on screen
    win.update_idletasks()
    sw = win.winfo_screenwidth()
    sh = win.winfo_screenheight()
    x = (sw - win_w) // 2
    y = (sh - win_h) // 2
    win.geometry(f"+{x}+{y}")

    # run modal mainloop
    try:
        win.grab_set()
    except Exception:
        pass
    try:
        root.deiconify()
        win.focus_force()
        root.mainloop()
    except Exception:
        # if mainloop crashes, ensure we still send a result
        pass

    # after mainloop ends, send result back
    try:
        conn.send(bool(result["value"]))
    except Exception:
        pass
    try:
        conn.close()
    except Exception:
        pass
    try:
        root.destroy()
    except Exception:
        pass

def stw(
    title: str,
    desc: str,
    pros,
    cons,
    minutes: int,
    execute_text: str = "Execute",
    cancel_text: str = "Cancel",
    win_size: Tuple[int,int] = (660, 880),
) -> bool:
    """
    Revised stw() — Pros and Cons are shown in two distinct sections (separate headers/frames).
    Keeps multiprocessing fallback for non-main-thread calls. Window title "nPhoneKIT".
    """
    #pros = ["test", "test2"]
    # normalize
    if pros is None:
        pros = []
    if cons is None:
        cons = []
    win_w, win_h = win_size
    win_h = 500

    # If caller not main thread -> spawn child (reuse existing worker implementation)
    if threading.current_thread() is not threading.main_thread():
        parent_conn, child_conn = multiprocessing.Pipe(duplex=False)
        p = multiprocessing.Process(
            target=_stw_worker,
            args=(child_conn, title, desc, pros, cons, minutes, execute_text, cancel_text, win_w, win_h),
        )
        p.start()
        child_conn.close()
        try:
            res = parent_conn.recv()
        except EOFError:
            res = False
        finally:
            try:
                parent_conn.close()
            except Exception:
                pass
            p.join(timeout=0.1)
            if p.is_alive():
                try:
                    p.terminate()
                except Exception:
                    pass
        return bool(res)

    # Main-thread path: inline dialog without extra empty root
    created_root = False
    parent_root = tk._default_root
    if parent_root is None:
        root = tk.Tk()
        created_root = True
        root.withdraw()
    else:
        root = parent_root

    # Colors & fonts
    bg = "#F5F7FB"
    card_bg = "#FFFFFF"
    text_primary = "#0F172A"
    text_secondary = "#374151"
    accent1 = "#2563EB"
    accent2 = "#0EA5E9"
    cons_red = "#B45309"
    pad = 14

    try:
        title_font = font.nametofont("TkHeadingFont").copy()
        title_font.configure(size=18, weight="bold", family="Segoe UI")
    except Exception:
        title_font = ("Segoe UI", 18, "bold")
    desc_font = ("Segoe UI", 12)
    list_font = ("Segoe UI", 12)
    emoji_font = ("Noto Color Emoji", 14)

    result = {"value": False}

    win = tk.Toplevel(root)
    win.title("nPhoneKIT")
    win.geometry(f"{win_w}x{win_h}")
    win.configure(bg=bg)
    win.resizable(False, False)

    def _on_close():
        result["value"] = False
        try:
            if created_root:
                win.quit()
            else:
                win.quit()
        except Exception:
            pass

    win.protocol("WM_DELETE_WINDOW", _on_close)

    content = tk.Frame(win, bg=bg)
    content.place(relx=0, rely=0, relwidth=1, relheight=1)
    left_w = int((win_w - pad*3) * 0.66)
    right_w = (win_w - pad*3) - left_w

    left_frame_outer = tk.Frame(content, bg=bg)
    left_frame_outer.place(x=pad, y=pad, width=left_w, height=win_h - pad*4 - 60)
    right_frame = tk.Frame(content, bg=bg)
    right_frame.place(x=pad + left_w + pad, y=pad, width=right_w, height=win_h - pad*4 - 60)

    # Scrollable left card (keeps content from overflowing)
    canvas = tk.Canvas(left_frame_outer, borderwidth=0, highlightthickness=0, bg=bg)
    vscroll = ttk.Scrollbar(left_frame_outer, orient="vertical", command=canvas.yview)
    canvas.configure(yscrollcommand=vscroll.set)
    vscroll.pack(side="right", fill="y")
    canvas.pack(side="left", fill="both", expand=True)
    left_card = tk.Frame(canvas, bg=card_bg)
    # set fixed width for left_card so wraplengths behave predictably
    left_card.pack_propagate(False)
    canvas.create_window((0,0), window=left_card, anchor="nw", width=left_w)

    def _on_config(e):
        canvas.configure(scrollregion=canvas.bbox("all"))
    left_card.bind("<Configure>", _on_config)

    # Title and description
    lbl_title = tk.Label(left_card, text=title, font=title_font, bg=card_bg, fg=text_primary, wraplength=left_w-40, justify="left")
    lbl_title.pack(anchor="w", pady=(12,6), padx=12)
    desc_lbl = tk.Label(left_card, text=desc, font=desc_font, bg=card_bg, fg=text_secondary, wraplength=left_w-40, justify="left")
    desc_lbl.pack(anchor="w", pady=(0,10), padx=12)

    # Horizontal separator
    sep1 = ttk.Separator(left_card, orient="horizontal")
    sep1.pack(fill="x", padx=12, pady=(4,10))

    # --- PROS SECTION (distinct frame) ---
    pros_frame = tk.Frame(left_card, bg=card_bg)
    pros_frame.pack(fill="x", padx=12, pady=(0,8))

    pros_hdr = tk.Label(pros_frame, text="PROS", font=("Segoe UI", 13, "bold"), bg=card_bg, fg=text_primary, anchor="w")
    pros_hdr.pack(anchor="w", pady=(0,6))

    # If no pros provided, show subtle 'None' label
    if not pros:
        none_lbl = tk.Label(pros_frame, text="(None)", font=list_font, bg=card_bg, fg=text_secondary, wraplength=left_w-40, justify="left")
        none_lbl.pack(anchor="w", pady=2)
    else:
        for p in pros:
            # Each item gets its own row with an emoji label and a text label that wraps
            item_row = tk.Frame(pros_frame, bg=card_bg)
            item_row.pack(fill="x", anchor="w", pady=4)
            em = tk.Label(item_row, text="✅", font=emoji_font, bg=card_bg)
            em.pack(side="left", anchor="n", padx=(0,6))
            txt = tk.Label(item_row, text=str(p), font=list_font, bg=card_bg, fg="#0B1720",
                           wraplength=left_w-80, justify="left", anchor="w")
            txt.pack(side="left", anchor="w", fill="x", expand=True)

    # separator between pros and cons
    sep2 = ttk.Separator(left_card, orient="horizontal")
    sep2.pack(fill="x", padx=12, pady=(10,10))

    # --- CONS SECTION (distinct frame) ---
    cons_frame = tk.Frame(left_card, bg=card_bg)
    cons_frame.pack(fill="x", padx=12, pady=(0,12))

    cons_hdr = tk.Label(cons_frame, text="CONS", font=("Segoe UI", 13, "bold"), bg=card_bg, fg=text_primary, anchor="w")
    cons_hdr.pack(anchor="w", pady=(0,6))

    if not cons:
        none_lbl2 = tk.Label(cons_frame, text="(None)", font=list_font, bg=card_bg, fg=text_secondary, wraplength=left_w-40, justify="left")
        none_lbl2.pack(anchor="w", pady=2)
    else:
        for c in cons:
            item_row = tk.Frame(cons_frame, bg=card_bg)
            item_row.pack(fill="x", anchor="w", pady=4)
            em = tk.Label(item_row, text="❌", font=emoji_font, bg=card_bg)
            em.pack(side="left", anchor="n", padx=(0,6))
            txt = tk.Label(item_row, text=str(c), font=list_font, bg=card_bg, fg=cons_red,
                           wraplength=left_w-80, justify="left", anchor="w")
            txt.pack(side="left", anchor="w", fill="x", expand=True)

    # finalize scroll region
    left_card.update_idletasks()
    canvas.configure(scrollregion=canvas.bbox("all"))
    canvas.bind_all("<MouseWheel>", lambda e: canvas.yview_scroll(int(-1*(e.delta/120)), "units"), add="+")

    # Buttons area
    btn_area = tk.Frame(content, bg=bg)
    btn_area.place(x=pad, y=win_h - pad - 56, width=win_w - pad*2, height=56)

    def _on_cancel_local():
        result["value"] = False
        try:
            if created_root:
                win.quit()
            else:
                win.quit()
        except Exception:
            pass

    cancel_btn = tk.Button(btn_area, text=cancel_text, command=_on_cancel_local,
                           bg=card_bg, fg=text_secondary, bd=1, relief="solid", padx=12, pady=6)
    cancel_btn.place(x=pad, y=6, width=120, height=44)

    # execute gradient button (canvas)
    exec_canvas = tk.Canvas(btn_area, bd=0, highlightthickness=0)
    exec_canvas.place(x=win_w - pad - 160, y=6, width=160, height=44)
    def _draw_gradient_canvas(cnv, x0, y0, x1, y1, color1, color2, steps=48):
        def _hex_to_rgb(h):
            h = h.lstrip("#")
            return tuple(int(h[i:i+2], 16) for i in (0,2,4))
        def _rgb_to_hex(rgb):
            return "#{:02x}{:02x}{:02x}".format(*rgb)
        c1 = _hex_to_rgb(color1)
        c2 = _hex_to_rgb(color2)
        width = x1 - x0
        for i in range(steps):
            t = i / steps
            r = int(c1[0] + (c2[0]-c1[0]) * t)
            g = int(c1[1] + (c2[1]-c1[1]) * t)
            b = int(c1[2] + (c2[2]-c1[2]) * t)
            cnv.create_rectangle(x0 + i*(width/steps), y0, x0 + (i+1)*(width/steps), y1, outline="", fill=_rgb_to_hex((r,g,b)))
    _draw_gradient_canvas(exec_canvas, 0, 0, 160, 44, accent1, accent2, steps=64)
    exec_canvas.create_text(80, 22, text=execute_text, fill="white", font=("Segoe UI", 11, "bold"))

    def _on_execute_local(event=None):
        result["value"] = True
        try:
            if created_root:
                win.quit()
            else:
                win.quit()
        except Exception:
            pass
    exec_canvas.bind("<Button-1>", _on_execute_local)
    exec_canvas.bind("<Return>", _on_execute_local)

    # Clock area (right)
    clock_w = right_w - 24
    clock_h = (win_h - pad*4 - 60)//2
    clock_canvas = tk.Canvas(right_frame, width=clock_w, height=clock_h, bg=card_bg, bd=0, highlightthickness=0)
    clock_canvas.pack(pady=(0,8))

    def _draw_clock_local():
        clock_canvas.delete("all")
        w = max(1, clock_canvas.winfo_width())
        h = max(1, clock_canvas.winfo_height())
        cx = w//2
        cy = h//2
        radius = int(min(w,h)*0.42)
        clock_canvas.create_oval(cx-radius, cy-radius, cx+radius, cy+radius, fill="#FFFFFF", outline="#D1D5DB", width=2)
        for i in range(60):
            ang = math.radians(i * 6)
            outer_x = cx + radius * math.sin(ang)
            outer_y = cy - radius * math.cos(ang)
            inner_len = radius * (0.90 if (i % 5 == 0) else 0.96)
            inner_x = cx + inner_len * math.sin(ang)
            inner_y = cy - inner_len * math.cos(ang)
            clock_canvas.create_line(inner_x, inner_y, outer_x, outer_y, fill="#9CA3AF", width=1)

        now = datetime.now()
        end = now + timedelta(minutes=minutes)
        minute_now = now.minute + now.second/60.0
        minute_end = end.minute + end.second/60.0
        span_clockwise = (minute_end - minute_now) % 60.0
        span_deg = span_clockwise * 6.0
        start_angle = 90 - (minute_now * 6.0)
        try:
            clock_canvas.create_arc(cx-radius+8, cy-radius+8, cx+radius-8, cy+radius-8, start=start_angle, extent=-span_deg, fill="#93C5FD", outline="")
        except Exception:
            clock_canvas.create_arc(int(cx-radius+8), int(cy-radius+8), int(cx+radius-8), int(cy+radius-8), start=int(start_angle), extent=int(-span_deg), fill="#93C5FD", outline="")
        def draw_hand(angle_deg, length_factor, color, width=4):
            rad = math.radians(angle_deg)
            x = cx + length_factor * radius * math.sin(rad)
            y = cy - length_factor * radius * math.cos(rad)
            clock_canvas.create_line(cx, cy, x, y, fill=color, width=width, capstyle="round")
        draw_hand(minute_now*6.0, 0.78, "#2563EB", 4)
        draw_hand(minute_end*6.0, 0.78, "#F59E0B", 4)
        clock_canvas.create_oval(cx-4, cy-4, cx+4, cy+4, fill="#111827", outline="")

    win.update_idletasks()
    _draw_clock_local()
    def _tick_local():
        try:
            _draw_clock_local()
            win.after(1000, _tick_local)
        except tk.TclError:
            pass
    win.after(1000, _tick_local)

    eta_time = (datetime.now() + timedelta(minutes=minutes)).strftime("%I:%M %p").lstrip("0")
    eta_label = tk.Label(right_frame, text=f"⏱ {minutes} min\n🕓 Ends at: {eta_time}", bg=card_bg, fg=text_primary, font=("Segoe UI", 12), justify="center")
    eta_label.pack(pady=(6,0))

    def _on_key(event):
        if event.keysym == "Return":
            _on_execute_local()
        elif event.keysym == "Escape":
            _on_cancel_local()
    win.bind_all("<Key>", _on_key)

    # center
    win.update_idletasks()
    sw = win.winfo_screenwidth()
    sh = win.winfo_screenheight()
    x = (sw - win_w) // 2
    y = (sh - win_h) // 2
    win.geometry(f"+{x}+{y}")

    try:
        win.grab_set()
    except Exception:
        pass

    # Run modal/blocking logic
    if created_root:
        try:
            root.focus_force()
            root.mainloop()
        except Exception:
            try:
                win.wait_window()
            except Exception:
                pass
    else:
        try:
            win.wait_window()
        except Exception:
            try:
                root.update()
            except Exception:
                pass

    # cleanup
    try:
        win.destroy()
    except Exception:
        pass
    if created_root:
        try:
            root.destroy()
        except Exception:
            pass

    return bool(result["value"])

class FastbootPartitionEraser:
    """
    This class attempts to erase FRP partition(s) on Motorola devices.
    """

    def __init__(self, fastboot_path='fastboot'):
        # Ensure the fastboot executable is available
        if not shutil.which(fastboot_path):
            raise FileNotFoundError(f"Fastboot binary '{fastboot_path}' not found in PATH.")
        self.fastboot = fastboot_path

    def _run(self, args):
        """
        Internal helper to run a fastboot command.
        Raises RuntimeError if command fails.
        """
        cmd = [self.fastboot] + args
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"Command {' '.join(cmd)} failed: {result.stderr.strip()}")
        return result.stdout.strip()

    def erase_config(self, device_id=None):
        """
        Erase the 'config' partition.
        Optionally specify a device serial with device_id.
        """
        args = []
        if device_id:
            args += ['-s', device_id]
        args += ['erase', 'config']
        return self._run(args)

    def erase_persist(self, device_id=None):
        """
        Erase the 'persist' partition.
        Optionally specify a device serial with device_id.
        """
        args = []
        if device_id:
            args += ['-s', device_id]
        args += ['erase', 'persist']
        return self._run(args)

    def erase_frp(self, device_id=None):
        """
        Erase the 'frp' partition.
        Optionally specify a device serial with device_id.
        """
        args = []
        if device_id:
            args += ['-s', device_id]
        args += ['erase', 'frp']
        return self._run(args)

    def wipe_data_cache(self, device_id=None):
        """
        Wipe data and cache partitions via 'fastboot -w'.
        Optionally specify a device serial with device_id.
        """
        args = []
        if device_id:
            args += ['-s', device_id]
        args += ['-w']
        return self._run(args)

# Check for updates

def check_for_update(): 
    try:
        repo = "nlckysolutions/nPhoneKIT"
        url = f"https://api.github.com/repos/{repo}/releases/latest"

        with urllib.request.urlopen(url, timeout=4) as response:
            data = json.loads(response.read().decode())

            latest_version_raw = data['tag_name']
            latest_version = data['tag_name'].lstrip("v")

            # If the tag is different then the current version, assume it's newer, and prompt update.

            # Based on the unicode "v", depending on whether it's normal or U+2174, prompt for normal update and FORCE for critical update
            
            # *************************************************************************
            # It's not reccomended to change this in order to bypass a critical update.
            # *************************************************************************

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
    ADB.send("devices")
    show_messagebox_at(500, 200, "nPhoneKIT", strings['adbMenu'])
    # Show user instructions to enable ADB mode

# ================================================
#  Simple functions to eliminate repetitive tasks
# ================================================

def rt(): # Flush the output buffer. May be deprecated and replaced soon with a new output collection method
    """if os_config == "LINUX": # Flush output buffer on different OSes
        os.system("sudo bash -c 'rm -f tmp_output.txt'") 
        os.system("sudo bash -c 'rm -f tmp_output_adb.txt'")
    elif os_config == "WINDOWS":
        os.system("del /F tmp_output.txt")
        os.system("del /F tmp_output_adb.txt")"""
    
    # Better rt() method + crossplatform + no errors
    for f in ["tmp_output.txt", "tmp_output_adb.txt"]:
        try:
            os.remove(f)
        except FileNotFoundError:
            pass

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

def lu(path="unlocks.json"):
    return json.loads(Path(path).read_text(encoding="utf-8"))
               
# =============================================
#  Unlocking methods for different devices
# =============================================

def frp_unlock_pre_aug2022(): # FRP unlock for pre-aug2022 security patch update
    methods = lu("unlocks.json")
    for m in methods:
        if m["id"] == "sam_pre_2022":
            picked = stw(m["title"], m["desc"], m["pros"], m["cons"], m["minutes"])
            if picked:
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
    methods = lu("unlocks.json")
    for m in methods:
        if m["id"] == "sam_2022_23":
            picked = stw(m["title"], m["desc"], m["pros"], m["cons"], m["minutes"])
            if picked:   
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
    methods = lu("unlocks.json")
    for m in methods:
        if m["id"] == "sam_2024":
            picked = stw(m["title"], m["desc"], m["pros"], m["cons"], m["minutes"])
            if picked:
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
    methods = lu("unlocks.json")
    for m in methods:
        if m["id"] == "lg_unlock":
            picked = stw(m["title"], m["desc"], m["pros"], m["cons"], m["minutes"])
            if picked:
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

def MotoFastbootFRP1():
    methods = lu("unlocks.json")
    for m in methods:
        if m["id"] == "moto_fastboot_frp_unlock":
            picked = stw(m["title"], m["desc"], m["pros"], m["cons"], m["minutes"])
            if picked:
                show_messagebox_at(200,200,"nPhoneKIT",strings["motoFastbootGuide"])
                # erase frp partitions upon fastboot access granted
                eraser = FastbootPartitionEraser()
                ecf_stat = eraser.erase_config()
                eps_stat = eraser.erase_persist()
                efr_stat = eraser.erase_frp()
                wdc_stat = eraser.wipe_data_cache()

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

def tkinput(title="Enter Value", text="Please enter a value:", placeholder="", ok_text="OK", cancel_text="Cancel"):
    result = {"value": None}

    def on_submit():
        val = entry.get()
        if val != placeholder:
            result["value"] = val
        popup.quit()

    def on_cancel():
        popup.quit()

    popup = tk.Tk()
    popup.title(title)
    popup.geometry("300x150")
    popup.resizable(False, False)

    label = tk.Label(popup, text=text)
    label.pack(pady=(15, 5))

    entry = tk.Entry(popup, width=30)
    entry.insert(0, placeholder)
    entry.pack(pady=5)
    entry.focus()
    entry.config(fg='grey')

    def on_focus_in(event):
        if entry.get() == placeholder:
            entry.delete(0, tk.END)
            entry.config(fg='black')

    def on_focus_out(event):
        if entry.get() == "":
            entry.insert(0, placeholder)
            entry.config(fg='grey')

    entry.bind("<FocusIn>", on_focus_in)
    entry.bind("<FocusOut>", on_focus_out)

    button_frame = tk.Frame(popup)
    button_frame.pack(pady=10)

    tk.Button(button_frame, text=ok_text, command=on_submit, width=10).pack(side=tk.LEFT, padx=5)
    tk.Button(button_frame, text=cancel_text, command=on_cancel, width=10).pack(side=tk.LEFT, padx=5)

    popup.protocol("WM_DELETE_WINDOW", on_cancel)
    popup.mainloop()
    popup.destroy()

    return result["value"]

def featureRequest():
    featureDesc = tkinput(
        title="nPhoneKIT",
        text="Feature Request:",
        placeholder="detailed feature description...",
        ok_text="Submit",
        cancel_text="Cancel"
    )

    if featureDesc is not None:
        print("Submitting request: ", featureDesc)
    else:
        print("Canceled.")

    data = {
        "timestamp": time.time(), 
        "uuid": str(get_public_hardware_uuid()),
        "feature": featureDesc,
        "phoneKITversion": version
    }

    try:
        response = requests.post(f"{FIREBASE_URL}/feature_requests.json", json=data)
        if response.status_code == 200:
            status = "Feature request submitted successfully!  OK"
        else:
            status = "Error: Feature request failed to send. Check your connection?  FAIL"
    except Exception as e:
        status = "Error: Feature request failed to send. Check your connection?  FAIL"
    print(status)

def bugReport():
    bugDesc = tkinput(
        title="nPhoneKIT",
        text="Bug Report:",
        placeholder="detailed bug description...",
        ok_text="Submit",
        cancel_text="Cancel"
    )

    if bugDesc is not None:
        print("Submitting request: ", bugDesc)
    else:
        print("Canceled.")

    data = {
        "timestamp": time.time(), 
        "uuid": str(get_public_hardware_uuid()),
        "bug": bugDesc,
        "phoneKITversion": version
    }

    try:
        response = requests.post(f"{FIREBASE_URL}/bug_reports.json", json=data)
        if response.status_code == 200:
            status = "Bug report submitted successfully!  OK"
        else:
            status = "Error: Bug report failed to send. Check your connection?  FAIL"
    except Exception as e:
        status = "Error: Bug report failed to send. Check your connection?  FAIL"
    print(status)

def setFakeBatteryPercent():
    percent = tkinput(
        title="nPhoneKIT",
        text="Fake Battery Percent:",
        placeholder="e.g: 101",
        ok_text="Submit",
        cancel_text="Cancel"
    )
    adbMenu()
    percent = percent.replace("%", "")
    print(f"Setting percentage to {percent}%...", end="")
    ADB.send(f"shell dumpsys battery set level {percent}")
    output = readOutput("ADB")
    if "unauthorized" in output:
        print("  FAIL (You need to authorize the device via the USB Debugging prompt. Unplugging and replugging the device may help with this.)")
    else:
        print("  OK  (Restarting your phone should undo this.)")

def resetBatteryPercent():
    adbMenu()
    print(f"Resetting percentage...", end="")
    ADB.send(f"shell dumpsys battery reset")
    output = readOutput("ADB")
    if "unauthorized" in output:
        print("  FAIL (You need to authorize the device via the USB Debugging prompt. Unplugging and replugging the device may help with this.)")
    else:
        print("  OK  (Restarting your phone should undo this.)")

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
        #font_big = QtGui.QFont(); font_big.setPointSize(12); font_big.setBold(True)
        font_big = QtGui.QFont("JetBrains Mono", 10)
        for i, (label, tooltip, fn) in enumerate(actions):
            btn = QtWidgets.QPushButton(label)
            btn.setToolTip(tooltip)
            btn.setMinimumHeight(44)
            btn.clicked.connect(partial(self.run_task, fn))
            # put into a card-like holder
            card = QtWidgets.QFrame(); card.setStyleSheet("QFrame { background: rgba(255,255,255,0.03); border: 1px solid #2A2A2A; border-radius: 12px; }")
            v = QtWidgets.QVBoxLayout(card); v.setContentsMargins(10,10,10,10)
            #btn.setFont(font_big)
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
        moto_actions = [
            (strings.get('motoFastbootUnlockFRP1','Fastboot-Based FRP Unlock'), strings.get('fbbFRPu1tooltip',''), MotoFastbootFRP1),
        ]
        mtk_actions = [
            (strings.get('mtkClientLabel','MTK Client GUI 🚀'), strings.get('mtkClientTooltip',''), mtkclient),
        ]
        android_actions = [
            (strings.get('crashReboot','Crash/Reboot ⚡'), strings.get('crashRebootInfo',''), reboot),
        ]
        adb_actions = [
            (strings.get('fbp','Set Fake Battery %'), strings.get('fbpInfo',''), setFakeBatteryPercent),
            (strings.get('rbp','Reset Fake Battery %'), strings.get('rbpInfo',''), resetBatteryPercent),
        ]
        feedback_actions = [
            (strings.get('featureRequest','Feature Request'), strings.get('featureRequestInfo',''), featureRequest),
            (strings.get('bugReport','Bug Report'), strings.get('bugReportInfo',''), bugReport),
        ]

        tabspec = [
            (strings.get('brandSamsung','Samsung'), samsung_actions),
            (strings.get('brandLg','LG'), lg_actions),
            (strings.get('brandMoto','Motorola'), moto_actions),
            (strings.get('brandMediatek','MediaTek'), mtk_actions),
            (strings.get('brandAndroid','Android'), android_actions),
            (strings.get('ADB', 'ADB'), adb_actions),
            (strings.get('feedback', 'Feedback'), feedback_actions),
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
    app.setFont(QFont("Sans Serif"))

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

# Check if nPhoneKIT will be able to use serial ports:

if os_config == "LINUX":
    if not check_serial_permissions():
        sys.exit(0)
elif os_config == "WINDOWS":
    if not is_root():
        if not debugMode:
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
