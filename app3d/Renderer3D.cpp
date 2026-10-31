#include "Renderer3D.h"

#include "Scene.h"

#include "engine/Paths.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>

using namespace scene;

namespace {

std::string tex(const char* name) { return engine::assetPath(std::string("textures/") + name); }

prim::MeshData merge(std::initializer_list<prim::MeshData> parts)
{
    prim::MeshData m;
    for (const prim::MeshData& p : parts)
        m.append(p);
    return m;
}

prim::MeshData moved(prim::MeshData m, glm::vec3 offset)
{
    for (prim::Vertex& v : m.vertices)
        v.position += offset;
    return m;
}

// Quad facing +z (towards the camera), slightly in front of the back wall; UVs cover [0, 1].
prim::MeshData windowQuad()
{
    const float z = kBackZ + 0.002f;
    prim::MeshData m;
    const glm::vec3 n(0.0f, 0.0f, 1.0f);
    m.addQuad({{kWindowMin.x, kWindowMin.y, z}, n, {0, 0}}, {{kWindowMax.x, kWindowMin.y, z}, n, {1, 0}},
              {{kWindowMax.x, kWindowMax.y, z}, n, {1, 1}}, {{kWindowMin.x, kWindowMax.y, z}, n, {0, 1}}, n);
    return m;
}

// Rectangular frame of four boxes around [lo, hi] in the plane z, protruding by depth.
prim::MeshData frame(glm::vec2 lo, glm::vec2 hi, float z, float width, float depth, float uv = 2.0f)
{
    return merge({
        prim::box({lo.x - width, lo.y - width, z}, {hi.x + width, lo.y, z + depth}, uv),
        prim::box({lo.x - width, hi.y, z}, {hi.x + width, hi.y + width, z + depth}, uv),
        prim::box({lo.x - width, lo.y, z}, {lo.x, hi.y, z + depth}, uv),
        prim::box({hi.x, lo.y, z}, {hi.x + width, hi.y, z + depth}, uv),
    });
}

// Colored band hugging the outer wall of the pot (a thin closed ring following the wall slope).
prim::MeshData potStripe(const prim::PotDims& p, float y0, float y1, float offset)
{
    auto r = [&](float y) { return p.bottomRadius + (p.topRadius - p.bottomRadius) * (y / p.height); };
    const float ri0 = r(y0) - 0.002f, ri1 = r(y1) - 0.002f;
    const float ro0 = r(y0) + offset, ro1 = r(y1) + offset;
    return prim::lathe({
                           {{ri0, y0}, {ro0, y0}}, // bottom
                           {{ro0, y0}, {ro1, y1}}, // outside
                           {{ro1, y1}, {ri1, y1}}, // top
                           {{ri1, y1}, {ri0, y0}}, // inside (hidden in the wall)
                       },
                       48);
}

} // namespace

Renderer3D::Renderer3D()
    : scene_(engine::shaderPath("scene.vs"), engine::shaderPath("scene.fs"))
    , beam_(engine::shaderPath("scene.vs"), engine::shaderPath("beam.fs"))
    , metal_(tex("metal.png"), true)
    , wall_(tex("wall.png"), true)
    , space_(tex("space.png"))
    , soilTex_(tex("soil_tile.png"), true)
{
    const glm::vec3 lo = kCabinetMin, hi = kCabinetMax;
    const float t = kPanel;
    // Outer shell: floor, roof, two sides (metal). The back wall is separate so it can use the wall texture.
    cabinet_ = GpuMesh(merge({
        prim::box({lo.x, lo.y, lo.z}, {hi.x, lo.y + t, hi.z}, 1.2f),
        prim::box({lo.x, hi.y - t, lo.z}, {hi.x, hi.y, hi.z}, 1.2f),
        prim::box({lo.x, lo.y + t, lo.z}, {lo.x + t, hi.y - t, hi.z}, 1.2f),
        prim::box({hi.x - t, lo.y + t, lo.z}, {hi.x, hi.y - t, hi.z}, 1.2f),
    }));
    cabinetInner_ = GpuMesh(prim::box({lo.x + t, lo.y + t, lo.z}, {hi.x - t, hi.y - t, lo.z + t}, 1.5f));
    windowQuad_ = GpuMesh(windowQuad());
    windowFrame_ = GpuMesh(merge({
        frame(kWindowMin, kWindowMax, kBackZ, 0.05f, 0.03f),
        prim::box({0.5f * (kWindowMin.x + kWindowMax.x) - 0.015f, kWindowMin.y, kBackZ},
                  {0.5f * (kWindowMin.x + kWindowMax.x) + 0.015f, kWindowMax.y, kBackZ + 0.02f}, 2.0f),
    }));

    potShell_ = GpuMesh(prim::potShell(kPot));
    prim::MeshData soil = prim::potSoil(kPot);
    for (prim::Vertex& v : soil.vertices)
        v.uv = glm::vec2(v.position.x, v.position.z) * 5.0f; // planar mapping keeps the grains round
    potSoil_ = GpuMesh(soil);
    potStripe_ = GpuMesh(potStripe(kPot, kPot.height * 0.52f, kPot.height * 0.62f, 0.004f));

    lampBody_ = GpuMesh(prim::box(kLampMin, kLampMax, 2.0f));
    lampMounts_ = GpuMesh(merge({
        prim::box({kLampMin.x + 0.15f, kLampMax.y, -0.53f}, {kLampMin.x + 0.19f, kCabinetMax.y - kPanel, -0.49f}),
        prim::box({kLampMax.x - 0.19f, kLampMax.y, -0.53f}, {kLampMax.x - 0.15f, kCabinetMax.y - kPanel, -0.49f}),
    }));
    lampStrip_ = GpuMesh(prim::box({kLampMin.x + 0.06f, kLampMin.y - 0.02f, kLampMin.z + 0.06f},
                                   {kLampMax.x - 0.06f, kLampMin.y, kLampMax.z - 0.06f}));

    const glm::vec3 tankTop(0.5f * (kTankMin.x + kTankMax.x), kTankMax.y, 0.5f * (kTankMin.z + kTankMax.z));
    tankBody_ = GpuMesh(merge({
        prim::box(kTankMin, kTankMax, 1.5f),
        moved(prim::cylinder(0.10f, 0.05f), tankTop),                              // filler cap
        moved(prim::cylinder(0.035f, kCabinetMax.y - kPanel - kTankMax.y - 0.05f), // pipe to the roof
              tankTop + glm::vec3(0.0f, 0.05f, 0.0f)),
    }));
    const char* stageFiles[3] = {"models/seed.obj", "models/sprout.obj", "models/mature.obj"};
    for (int i = 0; i < 3; ++i) {
        try {
            stageModels_[i] = Model(engine::assetPath(stageFiles[i]));
        } catch (const std::exception& e) {
            std::cerr << "Warning: " << e.what() << " - using a placeholder plant\n";
        }
    }
    fallbackPlant_ = GpuMesh(prim::sphere(0.05f));

    selectRim_ = GpuMesh(moved(prim::ring(kPot.topRadius + 0.004f, kPot.topRadius + 0.026f, 0.014f),
                               {0.0f, kPot.height - 0.018f, 0.0f}));
    selectFloor_ = GpuMesh(prim::ring(kPot.bottomRadius + 0.03f, kPot.bottomRadius + 0.075f, 0.004f));

    // Tank window: dark backing on the tank front, a water column (unit height, scaled by the water
    // ratio when drawn), a framed glass case in front of it.
    const float front = kTankMax.z;
    tankBacking_ = GpuMesh(prim::box({kTankWindowMin.x, kTankWindowMin.y, front}, {kTankWindowMax.x, kTankWindowMax.y, front + 0.01f}));
    water_ = GpuMesh(prim::box({kTankWindowMin.x + 0.006f, 0.0f, front + 0.012f}, {kTankWindowMax.x - 0.006f, 1.0f, front + 0.07f}));
    waterSurface_ = GpuMesh(prim::box({kTankWindowMin.x + 0.005f, -0.012f, front + 0.011f}, {kTankWindowMax.x - 0.005f, 0.0f, front + 0.071f}));
    tankGlass_ = GpuMesh(prim::box({kTankWindowMin.x - 0.004f, kTankWindowMin.y - 0.004f, front - 0.01f},
                                   {kTankWindowMax.x + 0.004f, kTankWindowMax.y + 0.004f, front + 0.08f}));
    prim::MeshData ticks;
    for (int k = 1; k < 4; ++k) {
        const float y = kTankWindowMin.y + (kTankWindowMax.y - kTankWindowMin.y) * float(k) / 4.0f;
        ticks.append(prim::box({kTankWindowMax.x + 0.004f, y - 0.004f, front}, {kTankWindowMax.x + 0.05f, y + 0.004f, front + 0.095f}));
    }
    tankFrame_ = GpuMesh(merge({frame(kTankWindowMin, kTankWindowMax, front, 0.035f, 0.09f), ticks}));

    // Light beam: from the lamp strip down to the floor, widening and following the light tilt.
    const float topY = kLampMin.y - 0.02f, bottomY = kFloorY + 0.003f;
    const glm::vec3 beamCorners[8] = {
        {kLampMin.x - 0.15f, bottomY, -1.15f}, {kLampMax.x + 0.15f, bottomY, -1.15f},
        {kLampMax.x + 0.15f, bottomY, -0.30f}, {kLampMin.x - 0.15f, bottomY, -0.30f},
        {kLampMin.x + 0.06f, topY, kLampMin.z + 0.06f}, {kLampMax.x - 0.06f, topY, kLampMin.z + 0.06f},
        {kLampMax.x - 0.06f, topY, kLampMax.z - 0.06f}, {kLampMin.x + 0.06f, topY, kLampMax.z - 0.06f},
    };
    beamVolume_ = GpuMesh(prim::hexahedron(beamCorners));
    droplet_ = GpuMesh(prim::sphere(1.0f, 16, 12));
}

int Renderer3D::pickPot(const garden::Garden& garden, const picking::Ray& ray)
{
    int best = -1;
    float bestT = 0.0f;
    for (int i = 0; i < kPotCount && i < garden.potCount(); ++i) {
        // Bounding cylinder of the pot and the plant currently growing in it.
        const float height = kPot.soilY() + kPlantHeight[int(garden.pot(i).stage)];
        const auto t = picking::intersectCylinder(ray, potBase(i), kPot.topRadius + 0.03f, std::max(height, kPot.height));
        if (t && (best < 0 || *t < bestT)) {
            best = i;
            bestT = *t;
        }
    }
    return best;
}

void Renderer3D::drawMesh(const GpuMesh& mesh, const glm::mat4& model, const Material& m)
{
    scene_.setMat4("uModel", model);
    scene_.setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    scene_.setBool("uUseTexture", m.texture != nullptr);
    if (m.texture)
        m.texture->bind(0);
    scene_.setVec3("uMaterial.color", m.color);
    scene_.setFloat("uMaterial.specular", m.specular);
    scene_.setFloat("uMaterial.shininess", m.shininess);
    scene_.setVec3("uMaterial.emission", m.emission);
    scene_.setFloat("uMaterial.alpha", m.alpha);
    mesh.draw();
}

void Renderer3D::draw(const garden::Garden& garden, const Frame& f)
{
    scene_.use();
    scene_.setMat4("uView", f.view);
    scene_.setMat4("uProjection", f.projection);
    scene_.setVec3("uViewPos", f.cameraPos);
    scene_.setInt("uTexture", 0);

    // Lamp spotlight; the weak ambient light keeps the cabinet visible when it is off.
    const bool on = garden.lamp().on;
    scene_.setBool("uLight.on", on);
    scene_.setVec3("uLight.position", kLightPos);
    scene_.setVec3("uLight.direction", glm::normalize(kLightDir));
    scene_.setFloat("uLight.cutOff", std::cos(glm::radians(kLightInnerDeg)));
    scene_.setFloat("uLight.outerCutOff", std::cos(glm::radians(kLightOuterDeg)));
    scene_.setVec3("uLight.color", glm::vec3(1.0f, 0.94f, 0.80f) * 1.6f);
    scene_.setFloat("uLight.constant", 1.0f);
    scene_.setFloat("uLight.linear", 0.09f);
    scene_.setFloat("uLight.quadratic", 0.032f);
    scene_.setVec3("uAmbient", on ? glm::vec3(0.24f, 0.26f, 0.32f) : glm::vec3(0.20f, 0.22f, 0.28f));

    drawCabinet();
    drawLamp(garden);
    drawTank(garden);
    drawPots(garden);
    drawDroplets(garden);
    drawTransparent(garden, f);
}

void Renderer3D::drawCabinet()
{
    const glm::mat4 I(1.0f);
    drawMesh(cabinet_, I, {&metal_, glm::vec3(0.80f), 0.20f, 24.0f});
    drawMesh(cabinetInner_, I, {&wall_, glm::vec3(1.0f), 0.1f, 16.0f});
    // The view to space glows on its own, so it stays visible with the lamp off.
    drawMesh(windowQuad_, I, {&space_, glm::vec3(0.0f), 0.0f, 1.0f, glm::vec3(1.0f)});
    drawMesh(windowFrame_, I, {&metal_, glm::vec3(0.7f), 0.5f, 48.0f});
}

void Renderer3D::drawLamp(const garden::Garden& garden)
{
    const glm::mat4 I(1.0f);
    const bool on = garden.lamp().on;
    drawMesh(lampMounts_, I, {nullptr, glm::vec3(0.30f, 0.33f, 0.40f), 0.4f, 32.0f});
    drawMesh(lampBody_, I, {&metal_, glm::vec3(0.55f, 0.57f, 0.62f), 0.5f, 48.0f});
    drawMesh(lampStrip_, I, on ? Material{nullptr, glm::vec3(0.0f), 0.0f, 1.0f, glm::vec3(1.0f, 0.95f, 0.75f)}
                                : Material{nullptr, glm::vec3(0.35f, 0.36f, 0.40f), 0.6f, 64.0f});
}

void Renderer3D::drawTank(const garden::Garden& garden)
{
    const glm::mat4 I(1.0f);
    drawMesh(tankBody_, I, {&metal_, glm::vec3(0.75f, 0.82f, 0.95f), 0.5f, 48.0f});
    drawMesh(tankBacking_, I, {nullptr, glm::vec3(0.05f, 0.07f, 0.11f), 0.3f, 32.0f});
    drawMesh(tankFrame_, I, {&metal_, glm::vec3(0.55f, 0.58f, 0.66f), 0.6f, 64.0f});

    // Water column through the window; its height is the tank ratio.
    const float ratio = std::clamp(garden.tank().ratio(), 0.0f, 1.0f);
    if (ratio > 0.0f) {
        const float h = (kTankWindowMax.y - kTankWindowMin.y) * ratio;
        const glm::mat4 column = glm::scale(glm::translate(I, {0.0f, kTankWindowMin.y, 0.0f}), {1.0f, h, 1.0f});
        drawMesh(water_, column, {nullptr, glm::vec3(0.10f, 0.38f, 0.85f), 0.8f, 96.0f, glm::vec3(0.02f, 0.10f, 0.25f)});
        drawMesh(waterSurface_, glm::translate(I, {0.0f, kTankWindowMin.y + h, 0.0f}),
                 {nullptr, glm::vec3(0.45f, 0.72f, 1.0f), 0.9f, 128.0f, glm::vec3(0.08f, 0.20f, 0.35f)});
    }
}

void Renderer3D::drawDroplets(const garden::Garden& garden)
{
    // Core gives each droplet a normalized height above the soil (1 = spawn, 0 = soil) and an
    // x offset in [-1, 1]; the depth offset is derived from x so the stream is not a flat sheet.
    const Material water{nullptr, glm::vec3(0.35f, 0.65f, 1.0f), 0.9f, 128.0f, glm::vec3(0.05f, 0.15f, 0.30f)};
    for (int i = 0; i < kPotCount && i < garden.potCount(); ++i) {
        for (const garden::Droplet& d : garden.pot(i).droplets) {
            if (!d.visible())
                continue;
            const glm::vec3 offset(d.x * kDropletSpread, kDropletRadius + d.y * kDropletFall,
                                   std::sin(d.x * 7.3f) * kDropletSpread * 0.7f);
            const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), soilCenter(i) + offset),
                                               glm::vec3(kDropletRadius, kDropletRadius * 1.5f, kDropletRadius));
            drawMesh(droplet_, model, water);
        }
    }
}

void Renderer3D::drawTransparent(const garden::Garden& garden, const Frame& f)
{
    // Transparent objects are drawn after all opaque ones, sorted back to front, without depth writes
    // in the color pass. Each object first writes only its depth, then draws color with GL_LEQUAL, so
    // only its nearest surface is blended. This keeps the picture the same with culling on or off.
    struct Item {
        float distance;
        bool beam;
    };
    const glm::vec3 glassCenter(0.5f * (kTankWindowMin.x + kTankWindowMax.x), 0.5f * (kTankWindowMin.y + kTankWindowMax.y), kTankMax.z + 0.035f);
    const glm::vec3 beamCenter(0.5f * (kLampMin.x + kLampMax.x), 0.5f * (kLampMin.y + kFloorY), -0.65f);
    Item items[2] = {{glm::length(f.cameraPos - glassCenter), false}, {glm::length(f.cameraPos - beamCenter), true}};
    std::sort(std::begin(items), std::end(items), [](const Item& a, const Item& b) { return a.distance > b.distance; });

    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    for (const Item& item : items) {
        if (item.beam && !garden.lamp().on)
            continue;
        for (int pass = 0; pass < 2; ++pass) {
            const bool depthOnly = pass == 0;
            glColorMask(!depthOnly, !depthOnly, !depthOnly, !depthOnly);
            glDepthMask(depthOnly ? GL_TRUE : GL_FALSE);
            if (item.beam) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); // light adds up
                beam_.use();
                beam_.setMat4("uView", f.view);
                beam_.setMat4("uProjection", f.projection);
                beam_.setMat4("uModel", glm::mat4(1.0f));
                beam_.setMat3("uNormalMatrix", glm::mat3(1.0f));
                beam_.setVec3("uViewPos", f.cameraPos);
                beam_.setVec3("uColor", glm::vec3(1.0f, 0.92f, 0.65f));
                beam_.setFloat("uAlpha", 0.22f);
                beam_.setFloat("uTopY", kLampMin.y - 0.02f);
                beam_.setFloat("uBottomY", kFloorY);
                beamVolume_.draw();
                scene_.use();
            } else {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                drawMesh(tankGlass_, glm::mat4(1.0f), {nullptr, glm::vec3(0.55f, 0.75f, 0.95f), 1.0f, 128.0f, glm::vec3(0.0f), 0.22f});
            }
        }
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer3D::drawPots(const garden::Garden& garden)
{
    for (int i = 0; i < kPotCount && i < garden.potCount(); ++i) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), potBase(i));
        // Selection: glowing rings at the rim and on the floor plus a faint tint; hover: dim rings.
        const bool selected = garden.selected() == i;
        const bool hovered = !selected && hovered_ == i;
        const glm::vec3 tint = selected ? glm::vec3(0.04f, 0.12f, 0.16f) : glm::vec3(0.0f);
        drawMesh(potShell_, model, {nullptr, glm::vec3(0.86f, 0.88f, 0.92f), 0.6f, 64.0f, tint});
        if (selected || hovered) {
            const glm::vec3 glow = selected ? glm::vec3(0.35f, 0.85f, 1.0f) : glm::vec3(0.12f, 0.30f, 0.36f);
            drawMesh(selectRim_, model, {nullptr, glm::vec3(0.0f), 0.0f, 1.0f, glow});
            drawMesh(selectFloor_, model, {nullptr, glm::vec3(0.0f), 0.0f, 1.0f, glow});
        }
        drawMesh(potStripe_, model, {nullptr, glm::vec3(0.27f, 0.55f, 0.86f), 0.5f, 48.0f});
        // Soil darkens with moisture.
        const float wet = garden.moistureRatio(i);
        const glm::vec3 soil = glm::mix(glm::vec3(0.66f, 0.50f, 0.33f), glm::vec3(0.30f, 0.19f, 0.11f), wet);
        drawMesh(potSoil_, model, {&soilTex_, soil, 0.05f, 8.0f});

        // Plant model for the current stage, standing on the soil; each pot is turned differently.
        if (garden.pot(i).hasPlant()) {
            glm::mat4 plant = glm::rotate(glm::translate(glm::mat4(1.0f), soilCenter(i)),
                                                glm::radians(40.0f + 110.0f * float(i)), glm::vec3(0.0f, 1.0f, 0.0f));
            plant = glm::scale(plant, glm::vec3(kPlantScale));
            drawPlant(garden.pot(i).stage, plant);
        }
    }
}

void Renderer3D::drawPlant(garden::Stage stage, const glm::mat4& model)
{
    const int index = stage == garden::Stage::Seed ? 0 : stage == garden::Stage::Sprout ? 1 : 2;
    const Model& m = stageModels_[index];
    if (m.empty()) {
        const float size = 0.4f + 0.6f * float(index);
        drawMesh(fallbackPlant_, glm::scale(glm::translate(model, glm::vec3(0.0f, 0.05f * size, 0.0f)), glm::vec3(size)),
                 {nullptr, glm::vec3(0.3f, 0.65f, 0.25f), 0.3f, 32.0f});
        return;
    }
    scene_.setMat4("uModel", model);
    scene_.setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    scene_.setVec3("uMaterial.emission", glm::vec3(0.0f));
    scene_.setFloat("uMaterial.alpha", 1.0f);
    m.Draw(scene_);
}
