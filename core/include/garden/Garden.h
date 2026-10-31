#pragma once

#include "Command.h"
#include "Config.h"
#include "Lamp.h"
#include "Pot.h"
#include "Tank.h"

#include <vector>

namespace garden {

// The shared logic model. Renderers only call select()/apply()/update() and read state.
class Garden {
public:
    explicit Garden(const Config& config = Config{});

    // Select a pot by index; -1 (or any out-of-range index) clears the selection.
    void select(int potIndex);
    int selected() const { return selected_; }
    bool hasSelection() const { return selected_ >= 0; }

    // Apply a user command. Pot commands act on the selected pot.
    CommandResult apply(Command c);
    CommandResult lastResult() const { return lastResult_; }

    // Advance the simulation: every pot grows independently, droplets fall.
    void update(float dt);

    int potCount() const { return static_cast<int>(pots_.size()); }
    const Pot& pot(int i) const { return pots_.at(static_cast<size_t>(i)); }
    const Pot* selectedPot() const { return hasSelection() ? &pots_[static_cast<size_t>(selected_)] : nullptr; }
    const Lamp& lamp() const { return lamp_; }
    const Tank& tank() const { return tank_; }
    const Config& config() const { return config_; }

    float moistureRatio(int i) const { return pot(i).moisture / config_.potCapacity; }
    bool isDry(int i) const { return pot(i).isGrowingStage() && pot(i).moisture <= 0.0f; }
    bool isGrowing(int i) const { return lamp_.on && pot(i).isGrowingStage() && pot(i).moisture > 0.0f; }

    // Warnings for the info panel (dry soil of the selected pot, empty tank).
    std::vector<Notice> notices() const;

private:
    CommandResult sow(Pot& p);
    CommandResult water(Pot& p);
    CommandResult harvest(Pot& p);
    void grow(Pot& p, float dt);
    void spawnDroplets(Pot& p);

    Config config_;
    std::vector<Pot> pots_;
    Lamp lamp_;
    Tank tank_;
    int selected_ = -1;
    CommandResult lastResult_ = CommandResult::Ok;
};

} // namespace garden
