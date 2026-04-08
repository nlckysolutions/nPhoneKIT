import os
import subprocess
import shutil
import sys

def run_command(cmd):
    print(f"Running: {cmd}")
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    for line in process.stdout:
        print(line, end="")
    process.wait()
    if process.returncode != 0:
        print(f"Error: Command failed with exit code {process.returncode}")
        # sys.exit(1) # Don't exit allow user to see output

def build():
    print("=== nPhoneKIT Build Automation ===")
    
    # 1. Setup Binaries
    print("\n[1/4] Setting up binaries (ADB/Fastboot)...")
    import setup_binaries
    setup_binaries.setup_platform_tools()

    # 2. Install Python Dependencies (just in case)
    print("\n[2/4] Ensuring dependencies are installed...")
    run_command("pip install pyinstaller PyQt5 pyserial requests")

    # 3. Run PyInstaller
    print("\n[3/4] Running PyInstaller...")
    # Use the spec file
    run_command("pyinstaller --clean nPhoneKIT.spec")

    # 4. Check for Inno Setup
    print("\n[4/4] Finalizing...")
    dist_path = os.path.join("dist", "nPhoneKIT")
    if os.path.exists(dist_path):
        print(f"SUCCESS: Standalone app built in {dist_path}")
        print("To create the installer, open 'installer_setup.iss' in Inno Setup and compile it.")
    else:
        print("FAILED: PyInstaller did not create the 'dist/nPhoneKIT' folder.")

if __name__ == "__main__":
    build()
