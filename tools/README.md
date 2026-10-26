# Flashing the RBoard on Fedora

Fedora sees this board as HID device `16c0:05df` (manufacturer `Robosapiens`, product `ROBOSAPIENS RBoard`), not a serial port. This project uses the BootloadHID uploader through PlatformIO's custom upload command.

The `bootloadHID` binary in this folder is a Linux x86-64 build of the official BootloadHID host utility. The RBoard uses the same VID/PID but custom USB name strings, so the included patch tells the host utility to match by VID/PID only. The matching source archive and rebuild script are included.

## Rebuild the uploader

Install build prerequisites once:

```bash
sudo dnf install gcc make python3 libusb-compat-0.1-devel
```

From this project folder, run:

```bash
python3 tools/build_bootloadhid.py
```

## Upload

Connect the RBoard and put it in its HID bootloader state. If required, use the board's reset/boot procedure, then confirm `lsusb` shows `16c0:05df`. In VS Code, run **PlatformIO: Upload**. The upload target calls `tools/bootloadHID -r` with the generated Intel HEX file; `-r` asks the bootloader to start the application after writing.

The bootloader reserves the upper 2 KB of ATmega8 flash. `platformio.ini` caps the application at 6144 bytes to protect that area.

If the uploader reports `Access to device denied`, add a Fedora udev rule and replug the board:

```bash
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05df", MODE="0660", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/70-rboard-bootloadhid.rules
sudo udevadm control --reload-rules
```

The original USB logs show one enumeration failure (`error -71`) followed by successful HID enumeration. If that repeats during upload, use a known-good data cable and direct USB port, then reset the board back into the HID bootloader state.

Source: Objective Development's BootloadHID 2012-12-08 archive, from <https://www.obdev.at/products/vusb/bootloadhid.html>.
