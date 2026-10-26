#include <Arduino.h>
#include <rainbow_trail.h>

namespace {

// Keep the renderer and single WS2812 framebuffer in static storage.
RainbowTrail rainbowTrail;

} // namespace

// Arduino framework entry points are called by the core, outside this TU.
// cppcheck-suppress unusedFunction
void setup()
{
    rainbowTrail.begin();
}

// cppcheck-suppress unusedFunction
void loop()
{
    rainbowTrail.update();
}
