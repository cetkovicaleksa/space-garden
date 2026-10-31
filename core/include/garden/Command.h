#pragma once

namespace garden {

enum class Command { Sow, Water, Harvest, ToggleLamp, RefillTank };

enum class CommandResult {
    Ok,
    NoSelection,     // command needs a selected pot
    PotNotEmpty,     // Sow on a pot that already has a plant
    NothingToWater,  // Water on an empty pot or a mature plant
    PotFull,         // soil already at capacity
    TankEmpty,       // no water left in the tank
    NotMature,       // Harvest before the plant is mature
    TankAlreadyFull, // Refill on a full tank
};

enum class Notice { DrySoil, TankEmpty };

const char* commandName(Command c);
const char* resultMessage(CommandResult r);
const char* noticeMessage(Notice n);

} // namespace garden
