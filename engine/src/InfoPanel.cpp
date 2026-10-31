#include "engine/InfoPanel.h"

#include "engine/Paths.h"
#include "engine/SpriteRenderer.h"

#include <algorithm>
#include <cmath>

namespace engine {

namespace {

const glm::vec4 kPanelBg{0.06f, 0.08f, 0.14f, 0.92f};
const glm::vec4 kPanelBorder{0.35f, 0.55f, 0.85f, 1.0f};
const glm::vec4 kText{0.92f, 0.95f, 1.0f, 1.0f};
const glm::vec4 kDim{0.60f, 0.68f, 0.80f, 1.0f};
const glm::vec4 kWarn{1.0f, 0.55f, 0.25f, 1.0f};
const glm::vec4 kMoisture{0.35f, 0.62f, 1.0f, 1.0f};
const glm::vec4 kGrowth{0.40f, 0.85f, 0.40f, 1.0f};
const glm::vec4 kWater{0.25f, 0.80f, 0.95f, 1.0f};
const glm::vec4 kLampOn{1.0f, 0.88f, 0.40f, 1.0f};

constexpr unsigned int kFontPx = 26;
constexpr float kMessageSeconds = 2.5f;

std::string percent(float ratio)
{
    return std::to_string(int(std::lround(std::clamp(ratio, 0.0f, 1.0f) * 100.0f))) + "%";
}

} // namespace

InfoPanel::InfoPanel()
    : text_(assetPath("fonts/DejaVuSans.ttf"), kFontPx)
    , bold_(assetPath("fonts/DejaVuSans-Bold.ttf"), kFontPx)
{
}

void InfoPanel::setProjection(const glm::mat4& projection)
{
    text_.setProjection(projection);
    bold_.setProjection(projection);
}

void InfoPanel::notify(garden::CommandResult result)
{
    if (result == garden::CommandResult::Ok)
        return;
    message_ = garden::resultMessage(result);
    messageTimer_ = kMessageSeconds;
}

void InfoPanel::update(float dt)
{
    if (messageTimer_ > 0.0f)
        messageTimer_ = std::max(0.0f, messageTimer_ - dt);
}

void InfoPanel::drawBar(SpriteRenderer& sprites, const std::string& label, float ratio, bool enabled,
                        glm::vec2 pos, glm::vec2 size, glm::vec4 color)
{
    const float labelW = size.x * 0.26f;
    const float pctW = size.x * 0.14f;
    const float barX = pos.x + labelW;
    const float barW = size.x - labelW - pctW;
    const float baseline = pos.y + (size.y - text_.capHeight(1.0f)) * 0.5f;

    text_.draw(label, pos.x, baseline, 1.0f, enabled ? kText : kDim);

    sprites.drawRect({barX, pos.y}, {barW, size.y}, {0.12f, 0.15f, 0.24f, 1.0f});
    if (enabled) {
        const float r = std::clamp(ratio, 0.0f, 1.0f);
        if (r > 0.0f)
            sprites.drawRect({barX, pos.y}, {barW * r, size.y}, color);
    }
    sprites.drawFrame({barX, pos.y}, {barW, size.y}, 2.0f, {0.30f, 0.38f, 0.55f, 1.0f});

    const std::string pct = enabled ? percent(ratio) : "-";
    text_.draw(pct, pos.x + size.x - text_.width(pct, 1.0f), baseline, 1.0f, enabled ? kText : kDim);
}

void InfoPanel::draw(const garden::Garden& garden, SpriteRenderer& sprites, glm::vec2 pos, glm::vec2 size)
{
    sprites.drawRect(pos, size, kPanelBg);
    sprites.drawFrame(pos, size, 3.0f, kPanelBorder);

    const float pad = 22.0f;
    const float line = text_.lineHeight(1.0f) * 1.3f;
    const float top = pos.y + size.y - pad;

    // --- Column 1: selected pot and bars ------------------------------------
    const float col1X = pos.x + pad;
    const float col1W = size.x * 0.42f;
    const garden::Pot* pot = garden.selectedPot();
    std::string header = pot ? "Pot " + std::to_string(garden.selected() + 1) + "  -  " + garden::stageName(pot->stage)
                             : "No pot selected (click a pot or press 1-3)";
    bold_.draw(header, col1X, top - text_.capHeight(1.0f), 1.0f, kText);

    const float barH = 26.0f;
    const float barGap = 16.0f;
    float y = top - line - barH - 8.0f;
    const bool plant = pot && pot->hasPlant();
    drawBar(sprites, "Moisture", pot ? garden.moistureRatio(garden.selected()) : 0.0f, plant, {col1X, y},
            {col1W, barH}, kMoisture);
    y -= barH + barGap;
    drawBar(sprites, "Growth", pot ? pot->progress : 0.0f, plant, {col1X, y}, {col1W, barH}, kGrowth);
    y -= barH + barGap;
    drawBar(sprites, "Water tank", garden.tank().ratio(), true, {col1X, y}, {col1W, barH}, kWater);

    // --- Column 2: lamp, status, warnings, feedback --------------------------
    const float col2X = pos.x + size.x * 0.48f;
    y = top - text_.capHeight(1.0f);
    const bool on = garden.lamp().on;
    sprites.drawRect({col2X, y - 2.0f}, {20.0f, 20.0f}, on ? kLampOn : glm::vec4(0.25f, 0.27f, 0.32f, 1.0f));
    bold_.draw(on ? "Lamp ON" : "Lamp OFF", col2X + 32.0f, y, 1.0f, on ? kLampOn : kDim);

    if (pot && pot->hasPlant()) {
        y -= line;
        const char* state = pot->stage == garden::Stage::Mature ? "Ready to harvest"
                            : garden.isGrowing(garden.selected()) ? "Growing"
                                                                  : "Growth paused";
        text_.draw(state, col2X, y, 1.0f, kText);
    }
    for (const std::string& s : status_) {
        y -= line;
        text_.draw(s, col2X, y, 1.0f, kDim);
    }
    for (garden::Notice n : garden.notices()) {
        y -= line;
        bold_.draw(std::string("! ") + garden::noticeMessage(n), col2X, y, 1.0f, kWarn);
    }
    if (messageTimer_ > 0.0f && !message_.empty()) {
        y -= line;
        const float alpha = std::min(1.0f, messageTimer_ / 0.5f);
        text_.draw(message_, col2X, y, 1.0f, {1.0f, 0.85f, 0.45f, alpha});
    }

    // --- Column 3: controls reminder ---------------------------------------
    const float col3X = pos.x + size.x * 0.78f;
    y = top - text_.capHeight(1.0f);
    bold_.draw("Controls", col3X, y, 0.9f, kText);
    for (const std::string& c : controls_) {
        y -= line * 0.78f;
        text_.draw(c, col3X, y, 0.8f, kDim);
    }
}

} // namespace engine
