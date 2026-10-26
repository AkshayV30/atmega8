#pragma once

#include <stdint.h>
#include <WS2812.h>

// Subpixel-smooth, bouncing rainbow head with an eased fading tail.
class RainbowTrail {
public:
    RainbowTrail();
    void begin();
    void update();

private:
    WS2812 ledStrip_;
    uint32_t lastFrameMs_;
    uint16_t headPositionQ8_;
    uint16_t huePhase_;
    int8_t direction_;

    void clearStrip();
    void fadeAndClearInactivePixels();
    void drawHead();
    static cRGB colorAtHue(uint16_t hue);
    static cRGB black();
};
