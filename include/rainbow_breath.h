#pragma once

#include <stdint.h>
#include <WS2812.h>

// Rainbow gradient with a smooth global breathing envelope.
class RainbowBreath {
public:
    RainbowBreath();
    void begin();
    void update();

private:
    WS2812 ledStrip_;
    uint32_t lastFrameMs_;
    uint16_t breathPhase_;
    uint16_t huePhase_;

    void clearStrip();
    uint8_t breathIntensity() const;
    void renderFrame(uint8_t intensity);
    static cRGB colorAtHue(uint16_t hue);
    static cRGB black();
};
