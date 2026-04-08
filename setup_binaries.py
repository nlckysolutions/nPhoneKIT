import os
import zipfile
import requests
import shutil

def download_file(url, filename):
    print(f"Downloading {url}...")
    response = requests.get(url, stream=True)
    with open(filename, 'wb') as f:
        for chunk in response.iter_content(chunk_size=8192):
            if chunk:
                f.write(chunk)
    print("Download complete.")

def setup_platform_tools():
    url = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip"
    zip_name = "platform-tools.zip"
    extract_dir = "temp_tools"
    bin_dir = "bin"

    if not os.path.exists(zip_name):
        download_file(url, zip_name)

    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)
    
    print("Extracting tools...")
    with zipfile.ZipFile(zip_name, 'r') as zip_ref:
        zip_ref.extractall(extract_dir)

    if not os.path.exists(bin_dir):
        os.makedirs(bin_dir)

    # Move only adb.exe, fastboot.exe and required DLLs
    src = os.path.join(extract_dir, "platform-tools")
    files_to_copy = ["adb.exe", "fastboot.exe", "AdbWinApi.dll", "AdbWinUsbApi.dll"]
    
    for f in files_to_copy:
        print(f"Copying {f} to {bin_dir}...")
        shutil.copy2(os.path.join(src, f), os.path.join(bin_dir, f))

    # Cleanup
    shutil.rmtree(extract_dir)
    os.remove(zip_name)
    print("Setup complete. Binaries are in the 'bin' folder.")

if __name__ == "__main__":
    setup_platform_tools()
