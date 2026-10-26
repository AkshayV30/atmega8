#include <rainbow_trail.h>
#include <led_config.h>

namespace {

const uint16_t kHueCycle = 1536U; // Six smooth RGB hue transitions.
const uint16_t kHueStepPerFrame = 5U;
const uint16_t kHeadStepQ8 = 32U; // 1/8 pixel per frame, about 8 pixels/s.
const uint8_t kTrailRetention = 243U; // Gentle per-frame decay for a long tail.
const uint16_t kFullScale = 255U;

cRGB fadePixel(cRGB pixel)
{
    pixel.r = static_cast<uint8_t>(
        static_cast<uint16_t>(pixel.r) * kTrailRetention / kFullScale);
    pixel.g = static_cast<uint8_t>(
        static_cast<uint16_t>(pixel.g) * kTrailRetention / kFullScale);
    pixel.b = static_cast<uint8_t>(
        static_cast<uint16_t>(pixel.b) * kTrailRetention / kFullScale);
    return pixel;
}

uint8_t scaledChannel(uint8_t channel, uint8_t coverage)
{
    const uint32_t numerator = static_cast<uint32_t>(channel) *
        rboard::kMaximumOutputChannel * coverage;
    return static_cast<uint8_t>(numerator / (kFullScale * kFullScale));
}

cRGB weightedColor(cRGB color, uint8_t coverage)
{
    color.r = scaledChannel(color.r, coverage);
    color.g = scaledChannel(color.g, coverage);
    color.b = scaledChannel(color.b, coverage);
    return color;
}

} // namespace

RainbowTrail::RainbowTrail()
    : ledStrip_(rboard::kPhysicalLedCount),
      lastFrameMs_(0),
      headPositionQ8_(0),
      huePhase_(0),
      direction_(1)
{
}

// Called once by Arduino setup(); Cppcheck does not follow framework entry.
// cppcheck-suppress unusedFunction
void RainbowTrail::begin()
{
    ledStrip_.setOutput(rboard::kLedDataPin);
    clearStrip();
    ledStrip_.sync();
    lastFrameMs_ = millis();
}

// Called continuously by Arduino loop().
// cppcheck-suppress unusedFunction
void RainbowTrail::update()
{
    const uint32_t nowMs = millis();
    if (nowMs - lastFrameMs_ < rboard::kFrameIntervalMs) {
        return;
    }
    lastFrameMs_ = nowMs;

    fadeAndClearInactivePixels();
    huePhase_ = static_cast<uint16_t>((huePhase_ + kHueStepPerFrame) % kHueCycle);
    drawHead();
    ledStrip_.sync();

    const uint16_t maximumPosition =
        static_cast<uint16_t>((rboard::kLedCount - 1U) << 8);
    const int32_t nextPosition = static_cast<int32_t>(headPositionQ8_) +
        static_cast<int32_t>(direction_) * kHeadStepQ8;
    if (nextPosition >= maximumPosition) {
        headPositionQ8_ = maximumPosition;
        direction_ = -1;
    } else if (nextPosition <= 0) {
        headPositionQ8_ = 0;
        direction_ = 1;
    } else {
        headPositionQ8_ = static_cast<uint16_t>(nextPosition);
    }
}

void RainbowTrail::clearStrip()
{
    const cRGB off = black();
    for (uint8_t index = 0; index < rboard::kPhysicalLedCount; ++index) {
        ledStrip_.set_crgb_at(index, off);
    }
}

void RainbowTrail::fadeAndClearInactivePixels()
{
    const cRGB off = black();
    for (uint8_t index = 0; index < rboard::kPhysicalLedCount; ++index) {
        if (index < rboard::kLedCount) {
            ledStrip_.set_crgb_at(index, fadePixel(ledStrip_.get_crgb_at(index)));
        } else {
            ledStrip_.set_crgb_at(index, off);
        }
    }
}

void RainbowTrail::drawHead()
{
    const uint8_t headIndex = static_cast<uint8_t>(headPositionQ8_ >> 8);
    const uint8_t fraction = static_cast<uint8_t>(headPositionQ8_ & 0xFFU);
    const cRGB headColor = colorAtHue(huePhase_);
    const uint8_t currentCoverage = static_cast<uint8_t>(255U - fraction);
    ledStrip_.set_crgb_at(headIndex, weightedColor(headColor, currentCoverage));

    if (fraction != 0U && headIndex + 1U < rboard::kLedCount) {
        ledStrip_.set_crgb_at(static_cast<uint8_t>(headIndex + 1U),
                              weightedColor(headColor, fraction));
    }
}

cRGB RainbowTrail::colorAtHue(uint16_t hue)
{
    const uint8_t segment = static_cast<uint8_t>(hue >> 8);
    const uint8_t rising = static_cast<uint8_t>(hue & 0xFFU);
    const uint8_t falling = static_cast<uint8_t>(255U - rising);
    cRGB color;

    switch (segment) {
    case 0: color = {255U, rising, 0U}; break;
    case 1: color = {falling, 255U, 0U}; break;
    case 2: color = {0U, 255U, rising}; break;
    case 3: color = {0U, falling, 255U}; break;
    case 4: color = {rising, 0U, 255U}; break;
    default: color = {255U, 0U, falling}; break;
    }
    return color;
}

cRGB RainbowTrail::black()
{
    cRGB off = {0U, 0U, 0U};
    return off;
}
