// Tiny console loop to play the garden logic without graphics.
// Commands: 1/2/3 select, s sow, w water, h harvest, l lamp, r refill, t <sec> wait, q quit
#include "garden/Garden.h"

#include <iostream>
#include <string>

using namespace garden;

static void print(const Garden& g)
{
    std::cout << "lamp " << (g.lamp().on ? "ON" : "off") << " | tank " << int(g.tank().ratio() * 100) << "%\n";
    for (int i = 0; i < g.potCount(); ++i) {
        const Pot& p = g.pot(i);
        std::cout << (g.selected() == i ? " > " : "   ") << "pot " << i + 1 << ": " << stageName(p.stage)
                  << "  moisture " << int(g.moistureRatio(i) * 100) << "%  growth " << int(p.progress * 100) << "%\n";
    }
    for (Notice n : g.notices())
        std::cout << " ! " << noticeMessage(n) << '\n';
}

int main()
{
    Garden g;
    std::string cmd;
    print(g);
    while (std::cout << "> " && std::cin >> cmd) {
        CommandResult r = CommandResult::Ok;
        if (cmd == "q") break;
        else if (cmd == "1" || cmd == "2" || cmd == "3") g.select(cmd[0] - '1');
        else if (cmd == "s") r = g.apply(Command::Sow);
        else if (cmd == "w") r = g.apply(Command::Water);
        else if (cmd == "h") r = g.apply(Command::Harvest);
        else if (cmd == "l") r = g.apply(Command::ToggleLamp);
        else if (cmd == "r") r = g.apply(Command::RefillTank);
        else if (cmd == "t") {
            float sec = 0;
            std::cin >> sec;
            for (float t = 0; t < sec; t += 0.1f) g.update(0.1f);
        }
        if (r != CommandResult::Ok) std::cout << resultMessage(r) << '\n';
        print(g);
    }
}
