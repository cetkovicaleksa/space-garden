#pragma once

#include "Droplet.h"

#include <vector>

namespace garden {

enum class Stage { Empty, Seed, Sprout, Mature };

const char* stageName(Stage s);

struct Pot {
    Stage stage    = Stage::Empty;
    float moisture = 0.0f;   // 0..Config::potCapacity
    float progress = 0.0f;   // 0..1 growth progress
    std::vector<Droplet> droplets;

    bool hasPlant() const { return stage != Stage::Empty; }
    bool isGrowingStage() const { return stage == Stage::Seed || stage == Stage::Sprout; }
};

} // namespace garden
