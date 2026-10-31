// Unit tests for garden_core. Minimal self-contained test runner, run with ctest.
#include "garden/Garden.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using namespace garden;

namespace {

int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("  FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

#define CHECK_NEAR(a, b) CHECK(std::fabs((a) - (b)) < 1e-3f)

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Register {
    Register(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); }
};

#define TEST(name)                                       \
    void name();                                         \
    static Register reg_##name(#name, name);             \
    void name()

// Simulate `seconds` in small fixed steps, like frames.
void run(Garden& g, float seconds, float step = 1.0f / 75.0f)
{
    for (float t = 0.0f; t < seconds; t += step)
        g.update(step);
}

Config fastConfig()
{
    Config c;
    c.growthRate = 0.1f;  // 10 s to mature
    c.drinkRate = 2.0f;
    c.waterPerPour = 25.0f;
    c.potCapacity = 100.0f;
    c.tankCapacity = 300.0f;
    return c;
}

} // namespace

TEST(initial_state)
{
    Garden g;
    CHECK(g.potCount() == 3);
    for (int i = 0; i < g.potCount(); ++i) {
        CHECK(g.pot(i).stage == Stage::Empty);
        CHECK_NEAR(g.pot(i).moisture, 0.0f);
    }
    CHECK(!g.lamp().on);
    CHECK(g.tank().full());
    CHECK(!g.hasSelection());
}

TEST(selection)
{
    Garden g;
    g.select(1);
    CHECK(g.selected() == 1);
    g.select(5);
    CHECK(g.selected() == -1);
    g.select(2);
    g.select(-1);
    CHECK(!g.hasSelection());
}

TEST(pot_commands_need_selection)
{
    Garden g;
    CHECK(g.apply(Command::Sow) == CommandResult::NoSelection);
    CHECK(g.apply(Command::Water) == CommandResult::NoSelection);
    CHECK(g.apply(Command::Harvest) == CommandResult::NoSelection);
    CHECK(g.lastResult() == CommandResult::NoSelection);
}

TEST(sow_only_on_empty)
{
    Garden g;
    g.select(0);
    CHECK(g.apply(Command::Sow) == CommandResult::Ok);
    CHECK(g.pot(0).stage == Stage::Seed);
    CHECK(g.apply(Command::Sow) == CommandResult::PotNotEmpty);
}

TEST(water_only_seed_or_sprout)
{
    Garden g;
    g.select(0);
    CHECK(g.apply(Command::Water) == CommandResult::NothingToWater);
    CHECK(g.tank().full());
}

TEST(full_growth_cycle)
{
    Garden g(fastConfig());
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    CHECK_NEAR(g.pot(0).moisture, 25.0f);
    CHECK_NEAR(g.tank().water, 275.0f);

    g.apply(Command::ToggleLamp);
    CHECK(g.lamp().on);
    CHECK(g.isGrowing(0));

    run(g, 4.0f); // progress ~0.4 -> Sprout
    CHECK(g.pot(0).stage == Stage::Sprout);
    CHECK(g.pot(0).moisture < 25.0f);

    run(g, 7.0f); // past 1.0 -> Mature
    CHECK(g.pot(0).stage == Stage::Mature);
    CHECK_NEAR(g.pot(0).progress, 1.0f);

    // Mature plants neither grow nor drink.
    const float m = g.pot(0).moisture;
    run(g, 3.0f);
    CHECK_NEAR(g.pot(0).moisture, m);
    CHECK(g.apply(Command::Water) == CommandResult::NothingToWater);

    CHECK(g.apply(Command::Harvest) == CommandResult::Ok);
    CHECK(g.pot(0).stage == Stage::Empty);
    CHECK_NEAR(g.pot(0).progress, 0.0f);
    CHECK_NEAR(g.pot(0).moisture, 0.0f);
}

TEST(harvest_only_mature)
{
    Garden g;
    g.select(0);
    CHECK(g.apply(Command::Harvest) == CommandResult::NotMature);
    g.apply(Command::Sow);
    CHECK(g.apply(Command::Harvest) == CommandResult::NotMature);
    CHECK(g.pot(0).stage == Stage::Seed);
}

TEST(growth_pauses_when_lamp_off)
{
    Garden g(fastConfig());
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    g.apply(Command::ToggleLamp);
    run(g, 2.0f);
    const float progress = g.pot(0).progress;
    const float moisture = g.pot(0).moisture;
    CHECK(progress > 0.0f);

    g.apply(Command::ToggleLamp); // off
    run(g, 5.0f);
    CHECK_NEAR(g.pot(0).progress, progress);
    CHECK_NEAR(g.pot(0).moisture, moisture);

    g.apply(Command::ToggleLamp); // on again -> resumes
    run(g, 1.0f);
    CHECK(g.pot(0).progress > progress);
}

TEST(growth_pauses_when_dry)
{
    Config c = fastConfig();
    c.waterPerPour = 4.0f; // 2 s of growth at drinkRate 2
    Garden g(c);
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    g.apply(Command::ToggleLamp);
    run(g, 5.0f);
    CHECK_NEAR(g.pot(0).moisture, 0.0f);
    CHECK_NEAR(g.pot(0).progress, 0.2f); // exactly 2 s of growth, not 5
    CHECK(g.isDry(0));
    CHECK(!g.isGrowing(0));

    bool dryNotice = false;
    for (Notice n : g.notices())
        dryNotice |= (n == Notice::DrySoil);
    CHECK(dryNotice);

    const float progress = g.pot(0).progress;
    run(g, 3.0f);
    CHECK_NEAR(g.pot(0).progress, progress);

    g.apply(Command::Water);
    run(g, 1.0f);
    CHECK(g.pot(0).progress > progress);
}

TEST(pour_clamped_by_capacity)
{
    Config c = fastConfig();
    c.potCapacity = 60.0f;
    Garden g(c);
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water); // 25
    g.apply(Command::Water); // 50
    g.apply(Command::Water); // clamped to 60
    CHECK_NEAR(g.pot(0).moisture, 60.0f);
    CHECK_NEAR(g.tank().water, 240.0f);
    CHECK(g.apply(Command::Water) == CommandResult::PotFull);
    CHECK_NEAR(g.tank().water, 240.0f);
}

TEST(pour_clamped_by_tank_and_empty_tank_blocks)
{
    Config c = fastConfig();
    c.tankCapacity = 30.0f;
    Garden g(c);
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water); // 25, tank 5
    CHECK(g.apply(Command::Water) == CommandResult::Ok); // only 5 left
    CHECK_NEAR(g.pot(0).moisture, 30.0f);
    CHECK(g.tank().empty());
    CHECK(g.apply(Command::Water) == CommandResult::TankEmpty);
    CHECK_NEAR(g.pot(0).moisture, 30.0f);

    bool tankNotice = false;
    for (Notice n : g.notices())
        tankNotice |= (n == Notice::TankEmpty);
    CHECK(tankNotice);

    CHECK(g.apply(Command::RefillTank) == CommandResult::Ok);
    CHECK(g.tank().full());
    CHECK(g.apply(Command::RefillTank) == CommandResult::TankAlreadyFull);
    CHECK(g.apply(Command::Water) == CommandResult::Ok);
}

TEST(unselected_pots_still_grow)
{
    Garden g(fastConfig());
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    g.select(2);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    g.select(1); // pot 1 stays empty and selected
    g.apply(Command::ToggleLamp);
    run(g, 2.0f);
    CHECK(g.pot(0).progress > 0.0f);
    CHECK(g.pot(2).progress > 0.0f);
    CHECK_NEAR(g.pot(1).progress, 0.0f);
    CHECK(g.pot(1).stage == Stage::Empty);
}

TEST(droplets_fall_and_disappear)
{
    Config c = fastConfig();
    c.dropletsPerPour = 4;
    c.dropletSpeed = 1.0f;
    c.dropletInterval = 0.1f;
    Garden g(c);
    g.select(0);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    CHECK(g.pot(0).droplets.size() == 4);
    CHECK(g.pot(0).droplets[0].visible());
    CHECK(!g.pot(0).droplets[3].visible());

    g.update(0.5f);
    // First droplet moved at constant speed: 1.0 - 0.5 * 1.0
    CHECK_NEAR(g.pot(0).droplets[0].y, 0.5f);
    for (const Droplet& d : g.pot(0).droplets) {
        CHECK(d.y <= 1.0f);
        CHECK(d.y > 0.0f);
    }

    run(g, 2.0f);
    CHECK(g.pot(0).droplets.empty());
}

TEST(droplets_independent_of_lamp)
{
    Garden g(fastConfig());
    g.select(1);
    g.apply(Command::Sow);
    g.apply(Command::Water);
    CHECK(!g.lamp().on);
    run(g, 3.0f);
    CHECK(g.pot(1).droplets.empty());
    CHECK_NEAR(g.pot(1).progress, 0.0f);
}

int main()
{
    int failedTests = 0;
    for (const TestCase& t : registry()) {
        const int before = g_failures;
        t.fn();
        const bool ok = g_failures == before;
        failedTests += ok ? 0 : 1;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", t.name);
    }
    std::printf("%zu tests, %d failed\n", registry().size(), failedTests);
    return failedTests == 0 ? 0 : 1;
}
