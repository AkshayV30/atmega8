# Robosapiens RSI-05 ATmega8 LED project

Firmware for the RBoard ATmega8. A smooth bouncing rainbow head paints a fading
trail across LEDs 0–30; LEDs 31–62 remain black.

## Project layout

```text
include/
  led_config.h                   Board, LED count, timing, brightness ceiling
src/
  main.cpp                       Arduino setup() and loop() entry points
  rainbow_trail.cpp              Subpixel head and fading-tail renderer
include/
  rainbow_trail.h                Effect class interface
tools/
  bootloadHID                    HID firmware uploader
  build_bootloadhid.py           Rebuilds the uploader from its archive
  README.md                      Upload and udev setup notes
platformio.ini                   Board, toolchain, library, and upload settings
SYSTEM_ARCHITECTURE.md           System and firmware flow diagram
```

`src/main.cpp` starts and services `RainbowTrail`. The WS2812 framebuffer is
the effect's only per-pixel storage.

## Animation behavior

- The firmware sends frames for all 63 pixels. LEDs 0–30 are active; LEDs 31–62
  are explicitly black on each frame.
- The head moves in eighth-pixel increments and reverses at either end. The two
  neighboring pixels are blended to smooth its motion.
- Each frame fades existing trail pixels before drawing the moving rainbow head.
- Output channels remain limited by `rboard::kMaximumOutputChannel`.
- The 16 ms frame gate targets about 60 updates per second.

## Brightness and power

The per-channel software ceiling is `rboard::kMaximumOutputChannel = 16`. Each
RGB output is clamped by scaling to this value. For reference, NeoPixel-class
RGB pixels are commonly estimated at up to 60 mA each at full-brightness white;
31 active pixels at 16/255 white scale would be about 117 mA by linear scaling. Actual
strip current and the RBoard's VCC/USB power-path capacity vary, so this estimate
is not a board safety rating. Use an appropriately rated, regulated 5 V supply
if the board or USB port is not rated for the strip load, join grounds, and keep
external +5 V separate from PC USB +5 V.

A 300–500 Ω series resistor between D9 and strip DIN is recommended to protect
the data input; it does not limit strip power current.

## Build and upload

Open the project root in VS Code with PlatformIO. Connect the RBoard and place it
into HID bootloader mode if its reset procedure requires it. Run
**PlatformIO: Upload** or **Project Tasks → atmega8 → General → Upload**.

From a terminal at the project root:

```bash
pio run -e atmega8 -t upload
```

The board enumerates as USB HID (`16c0:05df`), not as a serial port. PlatformIO
runs `tools/bootloadHID` through the custom upload command. Do not use `sudo pio`;
if USB access is denied, follow [tools/README.md](tools/README.md) and reconnect.

Build without uploading with `pio run -e atmega8`. The uploader rebuild script is
only needed when rebuilding `tools/bootloadHID`. Run
`python3 tools/build_bootloadhid.py` from the project root to rebuild it.

## Memory

The WS2812 library allocates three bytes per physical LED. A 63-pixel chain uses
189 bytes for RGB data, plus allocator overhead and stack. The ATmega8 has 1 KB
SRAM. The application is limited to 6144 bytes of flash to preserve the
bootloader region.
