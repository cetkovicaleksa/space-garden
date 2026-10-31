#include "garden/Command.h"
#include "garden/Pot.h"

namespace garden {

const char* stageName(Stage s)
{
    switch (s) {
    case Stage::Empty:  return "Empty";
    case Stage::Seed:   return "Seed";
    case Stage::Sprout: return "Sprout";
    case Stage::Mature: return "Mature";
    }
    return "?";
}

const char* commandName(Command c)
{
    switch (c) {
    case Command::Sow:        return "Sow";
    case Command::Water:      return "Water";
    case Command::Harvest:    return "Harvest";
    case Command::ToggleLamp: return "Toggle lamp";
    case Command::RefillTank: return "Refill tank";
    }
    return "?";
}

const char* resultMessage(CommandResult r)
{
    switch (r) {
    case CommandResult::Ok:              return "";
    case CommandResult::NoSelection:     return "Select a pot first.";
    case CommandResult::PotNotEmpty:     return "This pot already has a plant.";
    case CommandResult::NothingToWater:  return "Only a seed or a sprout can be watered.";
    case CommandResult::PotFull:         return "The soil is already soaked.";
    case CommandResult::TankEmpty:       return "The tank is empty. Refill it first.";
    case CommandResult::NotMature:       return "The plant is not mature yet.";
    case CommandResult::TankAlreadyFull: return "The tank is already full.";
    }
    return "";
}

const char* noticeMessage(Notice n)
{
    switch (n) {
    case Notice::DrySoil:   return "Dry soil - water the plant!";
    case Notice::TankEmpty: return "Water tank is empty!";
    }
    return "";
}

} // namespace garden
