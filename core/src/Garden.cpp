#include "garden/Garden.h"

#include <algorithm>

namespace garden {

Garden::Garden(const Config& config)
    : config_(config)
    , pots_(static_cast<std::size_t>(std::max(0, config.potCount)))
{
    tank_.capacity = config_.tankCapacity;
    tank_.water = config_.tankCapacity; // the tank starts full
}

void Garden::select(int potIndex)
{
    selected_ = (potIndex >= 0 && potIndex < potCount()) ? potIndex : -1;
}

CommandResult Garden::apply(Command c)
{
    CommandResult result = CommandResult::Ok;
    switch (c) {
    case Command::ToggleLamp:
        lamp_.on = !lamp_.on;
        break;
    case Command::RefillTank:
        if (tank_.full())
            result = CommandResult::TankAlreadyFull;
        else
            tank_.water = tank_.capacity;
        break;
    case Command::Sow:
    case Command::Water:
    case Command::Harvest:
        if (!hasSelection()) {
            result = CommandResult::NoSelection;
            break;
        }
        {
            Pot& p = pots_[static_cast<std::size_t>(selected_)];
            if (c == Command::Sow)        result = sow(p);
            else if (c == Command::Water) result = water(p);
            else                          result = harvest(p);
        }
        break;
    }
    lastResult_ = result;
    return result;
}

CommandResult Garden::sow(Pot& p)
{
    if (p.stage != Stage::Empty)
        return CommandResult::PotNotEmpty;
    p.stage = Stage::Seed;
    p.progress = 0.0f;
    return CommandResult::Ok;
}

CommandResult Garden::water(Pot& p)
{
    if (!p.isGrowingStage())
        return CommandResult::NothingToWater;
    if (tank_.empty())
        return CommandResult::TankEmpty;
    const float room = config_.potCapacity - p.moisture;
    if (room <= 0.0f)
        return CommandResult::PotFull;

    const float amount = std::min({config_.waterPerPour, tank_.water, room});
    p.moisture = std::min(config_.potCapacity, p.moisture + amount);
    tank_.water = std::max(0.0f, tank_.water - amount);
    spawnDroplets(p);
    return CommandResult::Ok;
}

CommandResult Garden::harvest(Pot& p)
{
    if (p.stage != Stage::Mature)
        return CommandResult::NotMature;
    p.stage = Stage::Empty;
    p.progress = 0.0f;
    p.moisture = 0.0f; // back to the initial, empty state
    return CommandResult::Ok;
}

void Garden::spawnDroplets(Pot& p)
{
    // Deterministic horizontal pattern so the stream looks scattered but tests stay stable.
    static constexpr float pattern[] = {0.0f, -0.45f, 0.35f, -0.15f, 0.55f, -0.6f, 0.2f, -0.3f};
    constexpr int patternSize = static_cast<int>(sizeof(pattern) / sizeof(pattern[0]));
    for (int i = 0; i < config_.dropletsPerPour; ++i) {
        Droplet d;
        d.x = pattern[i % patternSize];
        d.y = 1.0f;
        d.delay = static_cast<float>(i) * config_.dropletInterval;
        p.droplets.push_back(d);
    }
}

void Garden::grow(Pot& p, float dt)
{
    if (!lamp_.on || !p.isGrowingStage() || p.moisture <= 0.0f)
        return; // paused: progress is kept

    // Grow only for as long as the remaining moisture lasts within this step.
    const float t = config_.drinkRate > 0.0f ? std::min(dt, p.moisture / config_.drinkRate) : dt;
    p.progress = std::min(config_.matureThreshold, p.progress + config_.growthRate * t);
    p.moisture = std::max(0.0f, p.moisture - config_.drinkRate * t);

    if (p.progress >= config_.matureThreshold) {
        p.stage = Stage::Mature;
        p.progress = 1.0f;
    } else if (p.progress >= config_.sproutThreshold) {
        p.stage = Stage::Sprout;
    }
}

void Garden::update(float dt)
{
    if (dt <= 0.0f)
        return;
    for (Pot& p : pots_) {
        grow(p, dt);

        for (Droplet& d : p.droplets) {
            if (d.delay > 0.0f) {
                d.delay -= dt;
                if (d.delay > 0.0f)
                    continue;
                d.delay = 0.0f;
            }
            d.y -= config_.dropletSpeed * dt;
        }
        p.droplets.erase(std::remove_if(p.droplets.begin(), p.droplets.end(),
                                        [](const Droplet& d) { return d.visible() && d.y <= 0.0f; }),
                         p.droplets.end());
    }
}

std::vector<Notice> Garden::notices() const
{
    std::vector<Notice> out;
    if (hasSelection() && isDry(selected_))
        out.push_back(Notice::DrySoil);
    if (tank_.empty())
        out.push_back(Notice::TankEmpty);
    return out;
}

} // namespace garden
