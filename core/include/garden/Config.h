#pragma once

namespace garden {

// Tunable gameplay values. All amounts are in abstract "units", all rates per second.
struct Config {
    int   potCount          = 3;

    float potCapacity       = 100.0f; // max soil moisture per pot
    float tankCapacity      = 300.0f; // max water in the shared tank
    float waterPerPour      = 25.0f;  // water added by one Water command (before clamping)

    float growthRate        = 0.05f;  // progress per second while growing (0..1, ~20 s to mature)
    float drinkRate         = 4.0f;   // moisture consumed per second while growing

    float sproutThreshold   = 0.33f;  // progress at which Seed -> Sprout
    float matureThreshold   = 1.0f;   // progress at which Sprout -> Mature

    int   dropletsPerPour   = 6;      // droplets spawned by one pour
    float dropletSpeed      = 1.2f;   // normalized height per second (constant speed)
    float dropletInterval   = 0.12f;  // seconds between consecutive droplets of one pour
};

} // namespace garden
