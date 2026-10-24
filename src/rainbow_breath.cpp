#include "rainbow_breath.h"
#include <led_config.h>
#include <avr/pgmspace.h>

namespace {

// One signed Q7 sine cycle in flash keeps the ATmega8 SRAM free for runtime
// state and the WS2812 framebuffer.
const int8_t kSineQ7[64] PROGMEM = {
    0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
    127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122,
    -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81,
    -71, -60, -49, -37, -25, -12
};

// Advance about 171 phase units per frame: one full breath takes roughly six
// seconds at the configured 16 ms frame cadence.
const uint16_t kBreathPhaseStep = 171U;
const uint16_t kPhaseStartAtTrough = 0xC000U;
const uint16_t kSineRange = 254U;
const uint16_t kOutputScaleDivisor = 65025U; // 255 * 255
const uint16_t kHueCycle = 1536U; // Six 256-step RGB transitions.
// Five hue ticks per 16 ms frame produces a lively, roughly 4.9-second sweep.
const uint16_t kHueStepPerFrame = 5U;
const uint8_t kBreathFloor = 48U;
const uint8_t kBreathRange = 207U;
const int16_t kHueWaveDepth = 2;

int8_t sineQ7(uint16_t phase)
{
    const uint8_t tableIndex = static_cast<uint8_t>(phase >> 10);
    const uint8_t nextIndex = static_cast<uint8_t>((tableIndex + 1U) & 0x3FU);
    const uint8_t fraction = static_cast<uint8_t>((phase >> 2) & 0xFFU);
    const int8_t first = static_cast<int8_t>(pgm_read_byte(&kSineQ7[tableIndex]));
    const int8_t second = static_cast<int8_t>(pgm_read_byte(&kSineQ7[nextIndex]));
    const int16_t difference = static_cast<int16_t>(second - first);
    const int32_t interpolation =
        static_cast<int32_t>(difference) * fraction / 256L;
    return static_cast<int8_t>(first + interpolation);
}

uint8_t scaleOutput(uint8_t color, uint8_t intensity)
{
    const uint32_t scaled = static_cast<uint32_t>(color) * intensity *
                            rboard::kMaximumOutputChannel;
    return static_cast<uint8_t>(scaled / kOutputScaleDivisor);
}

} // namespace

RainbowBreath::RainbowBreath()
    : ledStrip_(rboard::kPhysicalLedCount),
      lastFrameMs_(0),
      breathPhase_(kPhaseStartAtTrough),
      huePhase_(0)
{
}

// Called once by Arduino setup(); its runtime call path is framework-owned.
// cppcheck-suppress unusedFunction
void RainbowBreath::begin()
{
    ledStrip_.setOutput(rboard::kLedDataPin);
    clearStrip();
    ledStrip_.sync();
    lastFrameMs_ = millis();
}

// Arduino invokes this method from loop(); Cppcheck analyzes this translation
// unit without following that framework-level runtime call path.
// cppcheck-suppress unusedFunction
void RainbowBreath::update()
{
    const uint32_t nowMs = millis();
    if (nowMs - lastFrameMs_ < rboard::kFrameIntervalMs) {
        return;
    }
    lastFrameMs_ = nowMs;

    breathPhase_ = static_cast<uint16_t>(breathPhase_ + kBreathPhaseStep);
    huePhase_ = static_cast<uint16_t>((huePhase_ + kHueStepPerFrame) % kHueCycle);

    renderFrame(breathIntensity());
    ledStrip_.sync();
}

void RainbowBreath::clearStrip()
{
    const cRGB off = black();
    for (uint8_t index = 0; index < rboard::kPhysicalLedCount; ++index) {
        ledStrip_.set_crgb_at(index, off);
    }
}

uint8_t RainbowBreath::breathIntensity() const
{
    const int16_t wave = sineQ7(breathPhase_);
    const uint16_t normalized = static_cast<uint16_t>(wave + 127);

    // A squared sine eases at both ends and spends less time near peak output.
    const uint32_t eased = static_cast<uint32_t>(normalized) * normalized * 255U;
    const uint8_t pulse = static_cast<uint8_t>(eased /
        (static_cast<uint32_t>(kSineRange) * kSineRange));
    return static_cast<uint8_t>(kBreathFloor +
        (static_cast<uint16_t>(pulse) * kBreathRange) / 255U);
}

void RainbowBreath::renderFrame(uint8_t intensity)
{
    const cRGB off = black();
    for (uint8_t index = 0; index < rboard::kPhysicalLedCount; ++index) {
        if (index >= rboard::kLedCount) {
            ledStrip_.set_crgb_at(index, off);
            continue;
        }

        const uint16_t gradientHue = static_cast<uint16_t>(
            static_cast<uint32_t>(index) * kHueCycle / rboard::kLedCount);
        const uint16_t spatialPhase = static_cast<uint16_t>(
            static_cast<uint32_t>(index) * 65536UL / rboard::kLedCount);
        const uint16_t waveOnePhase = static_cast<uint16_t>(
            spatialPhase + static_cast<uint32_t>(huePhase_) * 43U);
        const uint16_t waveTwoPhase = static_cast<uint16_t>(
            static_cast<uint32_t>(spatialPhase) * 2U -
            static_cast<uint32_t>(huePhase_) * 29U);
        const int16_t waveBlend = static_cast<int16_t>(sineQ7(waveOnePhase)) +
                                  sineQ7(waveTwoPhase);
        const int16_t hueWarp = waveBlend * kHueWaveDepth / 2;
        int32_t hue = static_cast<int32_t>(huePhase_) + gradientHue + hueWarp;
        hue %= kHueCycle;
        if (hue < 0) {
            hue += kHueCycle;
        }
        cRGB pixel = colorAtHue(static_cast<uint16_t>(hue));
        pixel.r = scaleOutput(pixel.r, intensity);
        pixel.g = scaleOutput(pixel.g, intensity);
        pixel.b = scaleOutput(pixel.b, intensity);
        ledStrip_.set_crgb_at(index, pixel);
    }
}

cRGB RainbowBreath::colorAtHue(uint16_t hue)
{
    const uint8_t segment = static_cast<uint8_t>(hue >> 8);
    const uint8_t ramp = static_cast<uint8_t>(hue & 0xFFU);
    const uint8_t falling = static_cast<uint8_t>(255U - ramp);
    cRGB color;

    // GRB field layout is handled by the WS2812 library; this struct is RGB.
    switch (segment) {
    case 0: color = {255U, ramp, 0U}; break;
    case 1: color = {falling, 255U, 0U}; break;
    case 2: color = {0U, 255U, ramp}; break;
    case 3: color = {0U, falling, 255U}; break;
    case 4: color = {ramp, 0U, 255U}; break;
    default: color = {255U, 0U, falling}; break;
    }
    return color;
}

cRGB RainbowBreath::black()
{
    cRGB color = {0U, 0U, 0U};
    return color;
}
