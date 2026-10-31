#include "Renderer2D.h"

#include "engine/Paths.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

using namespace layout;

namespace {
std::string tex(const char* name) { return engine::assetPath(std::string("textures/") + name); }
} // namespace

Renderer2D::Renderer2D()
    : title_(engine::assetPath("fonts/DejaVuSans-Bold.ttf"), 44)
    , space_(tex("space.png"))
    , metal_(tex("metal.png"), true)
    , wall_(tex("wall.png"), true)
    , pot_(tex("pot.png"))
    , soil_(tex("soil.png"))
    , seed_(tex("plant_seed.png"))
    , sprout_(tex("plant_sprout.png"))
    , mature_(tex("plant_mature.png"))
    , lamp_(tex("lamp.png"))
    , beam_(tex("beam.png"))
    , tank_(tex("tank.png"))
    , tankGlass_(tex("tank_glass.png"))
    , droplet_(tex("droplet.png"))
{
    const glm::mat4 projection = glm::ortho(0.0f, kWidth, 0.0f, kHeight);
    sprites_.setProjection(projection);
    title_.setProjection(projection);
    panel_.setProjection(projection);
}

void Renderer2D::beginFrame(int fbWidth, int fbHeight)
{
    // Clear the whole framebuffer (letterbox bars), then restrict drawing to the 16:9 canvas.
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    viewportScale_ = std::min(float(fbWidth) / kWidth, float(fbHeight) / kHeight);
    const glm::vec2 size(kWidth * viewportScale_, kHeight * viewportScale_);
    viewportOffset_ = 0.5f * (glm::vec2(float(fbWidth), float(fbHeight)) - size);
    glViewport(int(viewportOffset_.x), int(viewportOffset_.y), int(size.x), int(size.y));
}

glm::vec2 Renderer2D::toVirtual(double px, double py) const
{
    // Framebuffer y grows downwards, the canvas y grows upwards; letterbox bars are symmetric.
    const float x = (float(px) - viewportOffset_.x) / viewportScale_;
    const float y = (float(py) - viewportOffset_.y) / viewportScale_;
    return {x, kHeight - y};
}

int Renderer2D::pickPot(glm::vec2 p) const
{
    for (int i = 0; i < kPotCount; ++i)
        if (potHitRect(i).contains(p))
            return i;
    return -1;
}

void Renderer2D::draw(const garden::Garden& garden, float time)
{
    // Station background around the cabinet.
    sprites_.drawGradient({0.0f, 0.0f}, {kWidth, kHeight}, {0.03f, 0.04f, 0.08f, 1.0f}, {0.08f, 0.10f, 0.18f, 1.0f});
    title_.draw("Space Garden", kCabinet.pos.x, kCabinet.top() + 30.0f, 1.0f, {0.85f, 0.92f, 1.0f, 1.0f});

    drawCabinet();
    drawLamp(garden, time);
    drawTank(garden, time);
    drawPots(garden, time);
    drawLampBeam(garden, time); // transparent, drawn after the pots so they show through it

    panel_.draw(garden, sprites_, kPanel.pos, kPanel.size);
    nameTag_.draw(sprites_, {kWidth, kHeight});
}

void Renderer2D::drawCabinet()
{
    // Frame, back wall, window to space, shelf.
    sprites_.drawTiled(metal_, kCabinet.pos, kCabinet.size, {128.0f, 128.0f});
    sprites_.drawTiled(wall_, kInterior.pos, kInterior.size, {160.0f, 160.0f});

    const float rim = 14.0f;
    sprites_.drawRect(kWindow.pos - glm::vec2(rim), kWindow.size + glm::vec2(2.0f * rim), {0.55f, 0.60f, 0.70f, 1.0f});
    sprites_.draw(space_, kWindow.pos, kWindow.size);
    sprites_.drawFrame(kWindow.pos - glm::vec2(rim), kWindow.size + glm::vec2(2.0f * rim), 3.0f, {0.2f, 0.22f, 0.28f, 1.0f});
    sprites_.drawRect({kWindow.center().x - 4.0f, kWindow.pos.y}, {8.0f, kWindow.size.y}, {0.55f, 0.60f, 0.70f, 1.0f});

    // Inner shadow along the frame for depth.
    sprites_.drawGradient({kInterior.pos.x, kInterior.top() - 40.0f}, {kInterior.size.x, 40.0f}, {0, 0, 0, 0}, {0, 0, 0, 0.45f});
    sprites_.drawRect(kInterior.pos, {kInterior.size.x, kShelfHeight}, {0.42f, 0.45f, 0.52f, 1.0f});
    sprites_.drawRect({kInterior.pos.x, kFloorY - 4.0f}, {kInterior.size.x, 4.0f}, {0.62f, 0.66f, 0.74f, 1.0f});

    // Lamp mounts.
    for (float x : {kLamp.pos.x + 80.0f, kLamp.right() - 92.0f})
        sprites_.drawRect({x, kLamp.top() - 4.0f}, {12.0f, kInterior.top() - kLamp.top() + 4.0f}, {0.30f, 0.33f, 0.40f, 1.0f});
}

void Renderer2D::drawLamp(const garden::Garden& garden, float)
{
    if (garden.lamp().on) {
        // Bright lamp surface with a soft halo around the light strip.
        sprites_.drawRect({kLamp.pos.x + 10.0f, kLamp.pos.y - 8.0f}, {kLamp.size.x - 20.0f, 30.0f}, {1.0f, 0.92f, 0.6f, 0.25f});
        sprites_.draw(lamp_, kLamp.pos, kLamp.size, {1.0f, 0.97f, 0.85f, 1.0f});
        sprites_.drawRect({kLamp.pos.x + 26.0f, kLamp.pos.y + 4.0f}, {kLamp.size.x - 52.0f, 14.0f}, {1.0f, 0.96f, 0.75f, 1.0f});
    } else {
        sprites_.draw(lamp_, kLamp.pos, kLamp.size, {0.55f, 0.56f, 0.60f, 1.0f});
    }
}

void Renderer2D::drawLampBeam(const garden::Garden& garden, float time)
{
    if (!garden.lamp().on)
        return;
    // Semi-transparent light cone from the lamp down to the shelf (alpha blending).
    const float flicker = 0.96f + 0.04f * std::sin(time * 3.0f);
    const glm::vec2 pos{kLamp.pos.x + 10.0f, kFloorY};
    const glm::vec2 size{kLamp.size.x - 20.0f, kLamp.pos.y + 6.0f - kFloorY};
    sprites_.draw(beam_, pos, size, {1.0f, 0.93f, 0.62f, 0.30f * flicker});
}

void Renderer2D::drawTank(const garden::Garden& garden, float time)
{
    // Dark glass background, then water whose height is the tank ratio, then frame and glass highlight.
    sprites_.drawRect(kTankWater.pos, kTankWater.size, {0.06f, 0.08f, 0.12f, 1.0f});
    const float ratio = std::clamp(garden.tank().ratio(), 0.0f, 1.0f);
    if (ratio > 0.0f) {
        const float h = kTankWater.size.y * ratio;
        sprites_.drawGradient(kTankWater.pos, {kTankWater.size.x, h}, {0.08f, 0.32f, 0.75f, 1.0f}, {0.20f, 0.58f, 1.0f, 1.0f});
        const float wave = 2.0f * std::sin(time * 2.5f);
        sprites_.drawRect({kTankWater.pos.x, kTankWater.pos.y + h - 6.0f + wave * 0.5f}, {kTankWater.size.x, 6.0f},
                          {0.65f, 0.85f, 1.0f, 0.8f});
    }
    sprites_.draw(tank_, kTank.pos, kTank.size);
    sprites_.draw(tankGlass_, kTank.pos, kTank.size);
}

void Renderer2D::drawPots(const garden::Garden& garden, float)
{
    for (int i = 0; i < garden.potCount() && i < kPotCount; ++i) {
        const garden::Pot& state = garden.pot(i);
        const Rect pot = potRect(i);
        // Soil color darkens with moisture.
        const float wet = garden.moistureRatio(i);
        const glm::vec4 dry(0.66f, 0.50f, 0.33f, 1.0f), soaked(0.30f, 0.19f, 0.11f, 1.0f);
        const Rect soil{{pot.pos.x + 26.0f, soilY(i) - 16.0f}, {pot.size.x - 52.0f, 32.0f}};
        sprites_.draw(soil_, soil.pos, soil.size, glm::mix(dry, soaked, wet));

        // Plant sprite for the current stage, standing in the soil (its base hides behind the rim).
        const engine::Texture2D* plant = nullptr;
        switch (state.stage) {
        case garden::Stage::Seed:   plant = &seed_; break;
        case garden::Stage::Sprout: plant = &sprout_; break;
        case garden::Stage::Mature: plant = &mature_; break;
        case garden::Stage::Empty:  break;
        }
        if (plant) {
            const Rect pr = plantRect(i);
            sprites_.draw(*plant, pr.pos, pr.size);
        }

        // Falling droplets: core gives normalized height above the soil (1 = spawn, 0 = soil).
        for (const garden::Droplet& d : state.droplets) {
            if (!d.visible())
                continue;
            const glm::vec2 c{kPotCentersX[i] + d.x * kDropletSpreadX, soilY(i) + d.y * kDropletFall};
            sprites_.draw(droplet_, c - glm::vec2(0.5f * kDropletSize.x, 0.0f), kDropletSize, {1.0f, 1.0f, 1.0f, 0.95f});
        }

        sprites_.draw(pot_, pot.pos, pot.size);

        // Selection: colored frame around the pot and its plant; hover: faint frame.
        // The frame hugs the pot when empty and grows to include the plant.
        const Rect hit = state.hasPlant() ? potHitRect(i) : Rect{potRect(i).pos, {potRect(i).size.x, potRect(i).size.y + 24.0f}};
        const glm::vec2 pad(12.0f);
        if (garden.selected() == i)
            sprites_.drawFrame(hit.pos - pad, hit.size + 2.0f * pad, 5.0f, {0.35f, 0.85f, 1.0f, 1.0f});
        else if (hovered_ == i)
            sprites_.drawFrame(hit.pos - pad, hit.size + 2.0f * pad, 3.0f, {0.35f, 0.85f, 1.0f, 0.35f});
    }
}
