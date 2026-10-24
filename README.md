# Robosapiens RSI-05 ATmega8 LED project

Firmware for the RBoard ATmega8. The effect animates the first 10 WS2812 LEDs
with a flowing full-spectrum gradient and a visible breathing cycle.

## Project layout

```text
include/
  led_config.h                   Board, LED count, timing, brightness ceiling
src/
  main.cpp                       Arduino setup() and loop() entry points
  rainbow_breath.cpp             Effect renderer
include/
  rainbow_breath.h               Effect class interface
tools/
  bootloadHID                    HID firmware uploader
  build_bootloadhid.sh           Rebuilds the uploader from its archive
  README.md                      Upload and udev setup notes
platformio.ini                   Board, toolchain, library, and upload settings
SYSTEM_ARCHITECTURE.md           System and firmware flow diagram
```

`src/main.cpp` starts and services `RainbowBreath`. The WS2812 framebuffer is
the effect's only per-pixel storage.

## Animation behavior

- The firmware sends frames for all 63 pixels. LEDs 0–9 show the effect; LEDs
  10–62 are black on every frame.
- A full-spectrum gradient spans the active section. A traveling hue wave
  bends the gradient to produce evolving color combinations without abrupt
  palette changes.
- A flash-resident sine lookup table and squared integer easing create a smooth
  rise and fall without floating-point calculations. The pulse ranges from
  about 19% to full configured output for a clearly visible inhale/exhale while
  retaining a small nonzero trough. The breath takes roughly six seconds per
  cycle.
- The 16 ms frame gate targets about 60 updates per second.

## Brightness and power

The per-channel software ceiling is `rboard::kMaximumOutputChannel = 16`. Each
RGB output is clamped by scaling to this value. For reference, NeoPixel-class
RGB pixels are commonly estimated at up to 60 mA each at full-brightness white;
10 active pixels at 16/255 white scale would be about 38 mA by linear scaling. Actual
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
only needed when rebuilding `tools/bootloadHID`.

## Memory

The WS2812 library allocates three bytes per physical LED. A 63-pixel chain uses
189 bytes for RGB data, plus allocator overhead and stack. The ATmega8 has 1 KB
SRAM. The application is limited to 6144 bytes of flash to preserve the
bootloader region.
