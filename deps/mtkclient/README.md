# MTKClient
![Logo](mtkclient/gui/images/logo_256.png)

Just some mtk tool for exploitation, reading/writing flash and doing crazy stuff. 
For windows, you need to install the stock mtk port and the usbdk driver (see instructions below).
For linux, a patched kernel is only needed when using old kamakiri (see Setup folder) (except for read/write flash).

Once the mtk script is running, boot into brom mode by powering off device, press and hold either
vol up + power or vol down + power and connect the phone. Once detected by the tool,
release the buttons.

## MT678x, MT689x, MT688x, MT698x
- These chipsets use a new protocol called V6 and the bootrom is patched, thus you need a valid da via --loader option. 
- On some devices, preloader is deactivated, but you still use it by running "adb reboot edl".
- This only works with UNFUSED devices currently.
- For all devices with DAA, SLA and Remote-Auth activated no public solution currently exists (for various reasons).

## Credits
- kamakiri [xyzz]
- linecode exploit [chimera]
- Chaosmaster
- Geert-Jan Kreileman (GUI, design & fixes)
- All contributors

## Installation

### Use Re LiveDVD (everything ready to go, based on Ubuntu):
User: user, Password:user (based on Ubuntu 22.04 LTS)

[Live DVD V4](https://www.androidfilehost.com/?fid=15664248565197184488)

[Live DVD V4 Mirror](https://drive.google.com/file/d/10OEw1d-Ul_96MuT3WxQ3iAHoPC4NhM_X/view?usp=sharing)



## Install

### Linux  / Mac OS - (Ubuntu recommended, no patched kernel needed except for kamakiri)

#### Install python >=3.8, git and other deps

#### For Debian/Ubuntu
```
sudo apt install python3 git libusb-1.0-0 python3-pip libfuse2
```
#### For ArchLinux
```
(sudo) pacman -S  python python-pip git libusb fuse2
```
or
```
yay -S python python-pip git libusb fuse2
```

#### Grab files 
```
git clone https://github.com/bkerler/mtkclient
cd mtkclient
pip3 install -r requirements.txt
pip3 install .
```

#### Install rules
```
sudo usermod -a -G plugdev $USER
sudo usermod -a -G dialout $USER
sudo cp mtkclient/Setup/Linux/*.rules /etc/udev/rules.d
sudo udevadm control -R
```
Make sure to reboot after adding the user to dialout/plugdev. If the device
has a vendor interface 0xFF (like LG), make sure to add "blacklist qcaux" to
the "/etc/modprobe.d/blacklist.conf".

---------------------------------------------------------------------------------------------------------------

### Windows

#### Install python + git
- Install python >= 3.9 and git
- If you install python from microsoft store, "python setup.py install" will fail, but that step isn't required.
- WIN+R ```cmd```

#### Grab files and install
```
git clone https://github.com/bkerler/mtkclient
cd mtkclient
pip3 install -r requirements.txt
```

#### Get latest UsbDk 64-Bit
- Install normal MTK Serial Port driver (or use default Windows COM Port one, make sure no exclamation is seen)
- Get usbdk installer (.msi) from [here](https://github.com/daynix/UsbDk/releases/) and install it
- Test on device connect using "UsbDkController -n" if you see a device with 0x0E8D 0x0003
- Works fine under Windows 10 and 11 :D

---------------------------------------------------------------------------------------------------------------
### Use kamakiri (optional, only needed for mt6260 or older)

- For linux (kamakiri attack), you need to recompile your linux kernel using this kernel patch :
```
sudo apt-get install build-essential libncurses-dev bison flex libssl-dev libelf-dev libdw-dev
git clone https://git.kernel.org/pub/scm/devel/pahole/pahole.git
cd pahole && mkdir build && cd build && cmake .. && make && sudo make install
sudo mv /usr/local/libdwarves* /usr/local/lib/ && sudo ldconfig
```

```
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-`uname -r`.tar.xz
tar xvf linux-`uname -r`.tar.xz
cd linux-`uname -r`
patch -p1 < ../Setup/kernelpatches/disable-usb-checks-5.10.patch
cp -v /boot/config-$(uname -r) .config
make menuconfig
make
sudo make modules_install 
sudo make install
```

- These aren't needed for current ubuntu (as make install will do, just for reference):

```
sudo update-initramfs -c -k `uname -r`
sudo update-grub
```

See Setup/kernels for ready-to-use kernel setups


- Reboot

```
sudo reboot
```


---------------------------------------------------------------------------------------------------------------

## Usage

### Using MTKTools via the graphical user interface:
For the 'basics' you can use the GUI interface. This supports dumping partitions or the full flash for now. Run the following command:
```
python mtk_gui
```

### Run multiple commands
```bash
python mtk script run.example
```
See the file "[run.example](https://github.com/bkerler/mtkclient/blob/main/run.example)" on how to structure the script file

### Root the phone (Tested with android 9 - 12)

1. Dump boot and vbmeta
```
python mtk r boot,vbmeta boot.img,vbmeta.img
```

2. Reboot the phone
```
python mtk reset
```

3. Download patched magisk for mtk:
Download latest Magisk [here]([https://raw.githubusercontent.com/vvb2060/magisk_files/44ca9ed38c29e22fa276698f6c03bc1168df2c10/app-release.ap](https://github.com/topjohnwu/Magisk/releases)k)

4. Install on target phone
- you need to enable usb-debugging via Settings/About phone/Version, Tap 7x on build number
- Go to Settings/Additional settings/Developer options, enable "OEM unlock" and "USB Debugging"
- Install magisk apk
```
adb install app-release.apk
```
- accept auth rsa request on mobile screen of course to allow adb connection

5. Upload boot to /sdcard/Download
```
adb push boot.img /sdcard/Download
```

6. Start magisk, tap on Install, select boot.img from /sdcard/Download, then:
```
adb pull /sdcard/Download/[displayed magisk patched boot filename here]
mv [displayed magisk patched boot filename here] boot.patched
```

7. Do the steps needed in section "Unlock bootloader below"

8. Flash magisk-patched boot and empty vbmeta
```
python mtk w boot,vbmeta boot.patched,vbmeta.img.empty
```

9. Reboot the phone
```
python mtk reset
```

10. Disconnect usb cable and enjoy your rooted phone :)


### Boot to meta mode via payload

Example:

```
python mtk payload --metamode FASTBOOT
```

### Read efuses

Example:

```
python mtk da efuse
```

### Unlock bootloader

1. Erase metadata and userdata (and md_udc if existing):
```
python mtk e metadata,userdata,md_udc
```

2. Unlock bootloader:
```
python mtk da seccfg unlock
```
for relocking use:
```
python mtk da seccfg lock
```

3. Reboot the phone:
```
python mtk reset
```

and disconnect usb cable to let the phone reboot.

If you are getting a dm-verity error on Android 11, just press the power button,
then the device should boot and show a yellow warning about unlocked bootloader and
then the device should boot within 5 seconds.


### Read flash

Dump boot partition to filename boot.bin via preloader

```
python mtk r boot boot.bin
```

Dump boot partition to filename boot.bin via bootrom

```
python mtk r boot boot.bin [--preloader=Loader/Preloader/your_device_preloader.bin]
```


Dump preloader partition to filename preloader.bin via bootrom

```
python mtk r preloader preloader.bin --parttype=boot1 [--preloader=Loader/Preloader/your_device_preloader.bin]
```

Read full flash to filename flash.bin (use --preloader for brom)

```
python mtk rf flash.bin
```

Read full flash to filename flash.bin (use --preloader for brom) for IoT devices (MT6261/MT2301):

```
python mtk rf flash.bin --iot
```

Read flash offset 0x128000 with length 0x200000 to filename flash.bin (use --preloader for brom)

```
python mtk ro 0x128000 0x200000 flash.bin
```

Dump all partitions to directory "out". (use --preloader for brom)

```
python mtk rl out
```

Show gpt (use --preloader for brom)

```
python mtk printgpt
```


Mount the flash as a filesystem

```
python mtk fs /mnt/mtk
```

### Write flash
(use --preloader for brom)

Write filename boot.bin to boot partition

```
python mtk w boot boot.bin
```

Write filename flash.bin as full flash (currently only works in da mode)

```
python mtk wf flash.bin
```

Write all files in directory "out" to the flash partitions

```
python mtk wl out
```

write file flash.bin to flash offset 0x128000 with length 0x200000 (use --preloader for brom)

```
python mtk wo 0x128000 0x200000 flash.bin
```

### Erase flash

Erase boot partition
```
python mtk e boot
```

Erase boot sectors
```
python mtk es boot [sector count]
```

### DA commands:

Peek memory
```
python mtk da peek [addr in hex] [length in hex] [optional: -filename filename.bin for reading to file]
```

Poke memory
```
python mtk da poke [addr in hex] [data as hexstring or -filename for reading from file]
```

Read rpmb (Only xflash for now)
```
python mtk da rpmb r [will read to rpmb.bin]
```

Write rpmb [Currently broken, xflash only]
```
python mtk da rpmb w filename
```

Generate and display rpmb1-3 key
```
python mtk da generatekeys
```

Unlock / Lock bootloader
```
python mtk da seccfg [lock or unlock]
```

---------------------------------------------------------------------------------------------------------------

### Bypass SLA, DAA and SBC (using generic_patcher_payload)
`` 
python mtk payload
`` 
If you want to use SP Flash tool afterwards, make sure you select "UART" in the settings, not "USB".

### Dump preloader
- Device has to be in bootrom mode and preloader has to be intact on the device
```
python mtk dumppreloader [--ptype=["amonet","kamakiri","kamakiri2","hashimoto"]] [--filename=preloader.bin]
```

### Dump brom
- Device has to be in bootrom mode, or da mode has to be crashed to enter damode
- if no option is given, either kamakiri or da will be used (da for insecure targets)
- if "kamakiri" is used as an option, kamakiri is enforced
- Valid options are : "kamakiri" (via usb_ctrl_handler attack), "amonet" (via gcpu)
  and "hashimoto" (via cqdma)

```
python mtk dumpbrom --ptype=["amonet","kamakiri","hashimoto"] [--filename=brom.bin]
```

For to dump unknown bootroms, use brute option :
```
python mtk brute
```
If it's successful, please add an issue over here and append the bootrom in order to add full support.

---------------------------------------------------------------------------------------------------------------

### Crash da in order to enter brom

```
python mtk crash [--vid=vid] [--pid=pid] [--interface=interface]
```

### Read memory using patched preloader
- Boot in Brom or crash to Brom
```
python mtk peek [addr] [length] --preloader=patched_preloader.bin
```

### Run custom payload

```
python mtk payload --payload=payload.bin [--var1=var1] [--wdt=wdt] [--uartaddr=addr] [--da_addr=addr] [--brom_addr=addr]
```

---------------------------------------------------------------------------------------------------------------
## Stage2 usage
### Run python mtk stage (brom) or mtk plstage (preloader)

#### Run stage2 in bootrom
`` 
python mtk stage
`` 

#### Run stage2 in preloader
`` 
python mtk plstage
`` 

#### Run stage2 plstage in bootrom
- Boot in Brom or crash to Brom
```
python mtk plstage --preloader=preloader.bin
```

### Use stage2 tool


### Leave stage2 and reboot
`` 
python stage2 reboot
`` 

### Read rpmb in stage2 mode
`` 
python stage2 rpmb
`` 

### Read preloader in stage2 mode
`` 
python stage2 preloader
`` 

### Read memory as hex data in stage2 mode
`` 
python stage2 memread [start addr] [length]
`` 

### Read memory to file in stage2 mode
`` 
python stage2 memread [start addr] [length] --filename filename.bin
`` 

### Write hex data to memory in stage2 mode
`` 
python stage2 memwrite [start addr] --data [data as hexstring]
`` 

### Write memory from file in stage2 mode
`` 
python stage2 memwrite [start addr] --filename filename.bin
`` 

### Extract keys
`` 
python stage2 keys --mode [sej, dxcc]
`` 
For dxcc, you need to use plstage instead of stage

---------------------------------------------------------------------

### I have issues ....... please send logs and full console details !

- Run the mtk tool with --debugmode. Log will be written to log.txt (hopefully)

## Rules / Infos

### Chip details / configs
- Go to config/brom_config.py
- Unknown usb vid/pids for autodetection go to config/usb_ids.py

## Learning Resources 
[MTK Preloader](https://o0xmuhe.github.io/2022/03/05/MTK-Preloader-踩坑/)

[MOSEC-2022](https://o0xmuhe.github.io/2022/11/23/议题解读-MOSEC2022-MediAttack-break-the-boot-chain-of-MediaTek-SoC/)

[Dissecting MTK BROM Exploit](https://tinyhack.com/2021/01/31/dissecting-a-mediatek-bootrom-exploit/)

[Dumping Exynos BROM](https://fredericb.info/2020/06/exynos8890-bootrom-dump-dump-exynos-8890-bootrom-from-samsung-galaxy-s7.html)

[Rev Exynos BROM USB STACK ](https://fredericb.info/2020/06/reverse-engineer-usb-stack-of-exynos-bootrom.html#reverse-engineer-usb-stack-of-exynos-bootrom)

[Buffer Overflow In Huawei BROM USB STACK](https://labs.taszk.io/blog/post/bootrom_usb/)
