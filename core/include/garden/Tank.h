#pragma once

namespace garden {

struct Tank {
    float water    = 0.0f;
    float capacity = 0.0f;

    float ratio() const { return capacity > 0.0f ? water / capacity : 0.0f; }
    bool empty() const { return water <= 0.0f; }
    bool full() const { return water >= capacity; }
};

} // namespace garden
