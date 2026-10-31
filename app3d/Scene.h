#pragma once

#include "Primitives.h"

#include <glm/glm.hpp>

// World layout of the 3D scene, in meters. The cabinet stands on y = 0 with its open front at
// z = 0 and the back wall towards -z; x grows to the right as seen from the front.
namespace scene {

// Cabinet shell (outer bounds) and panel thickness.
constexpr glm::vec3 kCabinetMin{-1.6f, 0.0f, -1.3f};
constexpr glm::vec3 kCabinetMax{1.6f, 1.9f, 0.0f};
constexpr float kPanel = 0.06f;
constexpr float kFloorY = kCabinetMin.y + kPanel;      // top of the cabinet floor
constexpr float kBackZ = kCabinetMin.z + kPanel;       // inner side of the back wall

// Window to space on the back wall.
constexpr glm::vec2 kWindowMin{-1.25f, 0.78f}; // (x, y)
constexpr glm::vec2 kWindowMax{0.35f, 1.42f};

// Pots on the cabinet floor.
constexpr int kPotCount = 3;
constexpr float kPotX[kPotCount] = {-1.0f, -0.4f, 0.2f};
constexpr float kPotZ = -0.6f;
inline const prim::PotDims kPot{};
inline glm::vec3 potBase(int i) { return {kPotX[i], kFloorY, kPotZ}; }
inline glm::vec3 soilCenter(int i) { return potBase(i) + glm::vec3(0.0f, kPot.soilY(), 0.0f); }
// Height of the clickable volume above the soil for each stage (empty, seed, sprout, mature).
constexpr float kPlantHeight[4] = {0.05f, 0.08f, 0.26f, 0.58f};

// Shared lamp under the roof, above the pots, and its spotlight.
constexpr glm::vec3 kLampMin{-1.35f, 1.58f, -0.72f};
constexpr glm::vec3 kLampMax{0.55f, 1.70f, -0.30f};
// The light sits at the front of the lamp and is tilted slightly backwards, so it also reaches
// the front of the pots and plants instead of only their tops.
constexpr glm::vec3 kLightPos{-0.4f, 1.555f, -0.42f};
constexpr glm::vec3 kLightDir{0.0f, -1.0f, -0.22f}; // normalized in the shader
constexpr float kLightInnerDeg = 34.0f;
constexpr float kLightOuterDeg = 50.0f;

// Water tank at the right side, with a window in its front face showing the water column.
constexpr glm::vec3 kTankMin{0.85f, kFloorY, -0.95f};
constexpr glm::vec3 kTankMax{1.42f, 1.35f, -0.35f};
constexpr glm::vec2 kTankWindowMin{1.00f, 0.22f}; // (x, y) on the tank front face
constexpr glm::vec2 kTankWindowMax{1.27f, 1.18f};

// Droplets fall from this height above the soil (normalized y = 1) down to the soil (y = 0).
constexpr float kDropletFall = 0.55f;
constexpr float kDropletSpread = 0.09f; // stream half-width for x in [-1, 1]
constexpr float kDropletRadius = 0.018f;

// The camera always looks at the middle of the garden.
constexpr glm::vec3 kGardenCenter{-0.05f, 0.88f, -0.6f};

} // namespace scene
