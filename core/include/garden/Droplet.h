#pragma once

namespace garden {

// A falling water droplet above a pot.
// x: horizontal offset in [-1, 1] relative to the pot's soil width (0 = center).
// y: normalized height above soil in [0, 1] (1 = spawn height, 0 = soil).
// delay: seconds left before the droplet starts falling (it is hidden until then).
struct Droplet {
    float x     = 0.0f;
    float y     = 1.0f;
    float delay = 0.0f;

    bool visible() const { return delay <= 0.0f; }
};

} // namespace garden
