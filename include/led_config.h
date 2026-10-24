#pragma once

#include <Arduino.h>
#include <stdint.h>

// Board-specific and animation-wide settings shared by the firmware modules.
// Change these values here rather than duplicating hardware assumptions in
// the animation implementation.
namespace rboard {

// Number of pixels in the physical WS2812 chain. Pixels beyond the active
// animation region are explicitly written black so stale colors are cleared.
const uint8_t kPhysicalLedCount = 63;

// Only this leading part of the physical chain receives the rainbow effect.
const uint8_t kLedCount = 10;

// Arduino D9 maps to ATmega8 port B, bit 1 on the RSI-05 board.
const uint8_t kLedDataPin = 9;

// Minimum interval between animation frames. WS2812 transmission time also
// adds latency to the configured frame interval.
const unsigned long kFrameIntervalMs = 16UL;

// Modest software per-channel ceiling for the 10-pixel rainbow effect.
// This limits firmware output but does not certify the board's power path.
const uint8_t kMaximumOutputChannel = 16;

// Keep the configured pixel count within the available frame-buffer budget.
static_assert(kLedCount >= 2, "The animation requires at least two LEDs");
static_assert(kLedCount <= kPhysicalLedCount,
              "The active animation region cannot exceed the physical chain");
static_assert(kPhysicalLedCount <= 63,
              "The physical chain exceeds the configured project limit");
static_assert(kFrameIntervalMs > 0, "The frame interval must be nonzero");

} // namespace rboard
