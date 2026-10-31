#pragma once

#include <glm/glm.hpp>

// Scene layout of the 2D app in a fixed virtual resolution (origin bottom-left, y up).
// The virtual canvas is letterboxed into the real framebuffer, so layout is resolution-independent.
namespace layout {

constexpr float kWidth = 1920.0f;
constexpr float kHeight = 1080.0f;

struct Rect {
    glm::vec2 pos;  // bottom-left
    glm::vec2 size;

    bool contains(glm::vec2 p) const
    {
        return p.x >= pos.x && p.x <= pos.x + size.x && p.y >= pos.y && p.y <= pos.y + size.y;
    }
    glm::vec2 center() const { return pos + 0.5f * size; }
    float top() const { return pos.y + size.y; }
    float right() const { return pos.x + size.x; }
};

// Info panel below the cabinet.
constexpr Rect kPanel{{40.0f, 24.0f}, {1840.0f, 250.0f}};

// Cabinet seen from the front: metal frame around the interior.
constexpr Rect kCabinet{{140.0f, 296.0f}, {1640.0f, 700.0f}};
constexpr float kFrame = 36.0f;
constexpr Rect kInterior{{kCabinet.pos.x + kFrame, kCabinet.pos.y + kFrame},
                         {kCabinet.size.x - 2.0f * kFrame, kCabinet.size.y - 2.0f * kFrame}};
constexpr float kShelfHeight = 34.0f;
constexpr float kFloorY = kInterior.pos.y + kShelfHeight;

// Window to space on the back wall.
constexpr Rect kWindow{{430.0f, 600.0f}, {700.0f, 270.0f}};

// Three pots on the floor of the cabinet.
constexpr int kPotCount = 3;
constexpr glm::vec2 kPotSize{250.0f, 188.0f};
constexpr float kPotCentersX[kPotCount] = {420.0f, 780.0f, 1140.0f};
inline Rect potRect(int i) { return {{kPotCentersX[i] - 0.5f * kPotSize.x, kFloorY}, kPotSize}; }

// Soil strip at the top of the pot body and the plant sprite standing on it.
inline float soilY(int i) { return potRect(i).top() - 2.0f; }
constexpr glm::vec2 kPlantSize{230.0f, 288.0f};
inline Rect plantRect(int i) { return {{kPotCentersX[i] - 0.5f * kPlantSize.x, soilY(i) - 4.0f}, kPlantSize}; }
// Clickable area of a pot: pot body plus the plant above it.
inline Rect potHitRect(int i)
{
    const Rect p = potRect(i);
    return {p.pos, {p.size.x, plantRect(i).top() - p.pos.y}};
}

// Droplets fall from this height above the soil (normalized y = 1) down to the soil (y = 0).
constexpr float kDropletFall = 270.0f;
constexpr float kDropletSpreadX = 80.0f; // half-width of the stream for x in [-1, 1]
constexpr glm::vec2 kDropletSize{24.0f, 36.0f};

// Shared lamp under the cabinet roof, above the pots.
constexpr Rect kLamp{{250.0f, 868.0f}, {1060.0f, 64.0f}};

// Water tank at the right side of the cabinet.
constexpr Rect kTank{{1430.0f, kFloorY}, {250.0f, 500.0f}};
// Window of the tank in which the water level is visible (matches tank.png cut-out).
constexpr Rect kTankWater{{kTank.pos.x + kTank.size.x * (44.0f / 256.0f), kTank.pos.y + kTank.size.y * (60.0f / 512.0f)},
                          {kTank.size.x * (168.0f / 256.0f), kTank.size.y * (382.0f / 512.0f)}};

} // namespace layout
