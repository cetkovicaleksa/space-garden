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

    tankBacking_ = GpuMesh(merge({
        prim::box({kTankWindowMin.x, kTankWindowMin.y, kTankMax.z}, {kTankWindowMax.x, kTankWindowMax.y, kTankMax.z + 0.01f}),
    }));
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

void Renderer3D::drawTank(const garden::Garden&)
{
    const glm::mat4 I(1.0f);
    drawMesh(tankBody_, I, {&metal_, glm::vec3(0.75f, 0.82f, 0.95f), 0.5f, 48.0f});
    drawMesh(tankBacking_, I, {nullptr, glm::vec3(0.05f, 0.07f, 0.11f), 0.3f, 32.0f});
}

void Renderer3D::drawPots(const garden::Garden& garden)
{
    for (int i = 0; i < kPotCount && i < garden.potCount(); ++i) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), potBase(i));
        drawMesh(potShell_, model, {nullptr, glm::vec3(0.86f, 0.88f, 0.92f), 0.6f, 64.0f});
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
