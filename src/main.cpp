#include <Arduino.h>
#include <rainbow_breath.h>

namespace {

// Keep the renderer and single WS2812 framebuffer in static storage.
RainbowBreath rainbowBreath;

} // namespace

// Arduino framework entry points are called by the core, outside this TU.
// cppcheck-suppress unusedFunction
void setup()
{
    rainbowBreath.begin();
}

// cppcheck-suppress unusedFunction
void loop()
{
    rainbowBreath.update();
}
