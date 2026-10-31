#pragma once

#include "GpuMesh.h"
#include "OrbitCamera.h"
#include "Picking.h"

#include "engine/Texture.h"

#include <garden/Garden.h>

#include <learnopengl/model.h>
#include <learnopengl/shader.h>

#include <glm/glm.hpp>

// Perspective renderer of the Garden. Only reads the model; never changes it.
class Renderer3D {
public:
    Renderer3D();

    struct Frame {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPos;
        float time = 0.0f;
    };

    void draw(const garden::Garden& garden, const Frame& frame);
    void setHovered(int potIndex) { hovered_ = potIndex; }

    // Index of the nearest pot (with its plant) hit by the ray, or -1.
    static int pickPot(const garden::Garden& garden, const picking::Ray& ray);

private:
    struct Material {
        const engine::Texture2D* texture = nullptr; // nullptr = flat color
        glm::vec3 color{1.0f};
        float specular = 0.2f;
        float shininess = 32.0f;
        glm::vec3 emission{0.0f};
        float alpha = 1.0f;
    };

    void drawMesh(const GpuMesh& mesh, const glm::mat4& model, const Material& material);
    void drawCabinet();
    void drawPots(const garden::Garden& garden);
    void drawLamp(const garden::Garden& garden);
    void drawTank(const garden::Garden& garden);
    void drawPlant(garden::Stage stage, const glm::mat4& model);

    Shader scene_;

    engine::Texture2D metal_, wall_, space_, soilTex_;

    GpuMesh cabinet_, cabinetInner_, windowQuad_, windowFrame_;
    GpuMesh potShell_, potSoil_, potStripe_;
    GpuMesh lampBody_, lampMounts_, lampStrip_;
    GpuMesh tankBody_, tankBacking_;

    // Plant models for the seed, sprout and mature stages (Assimp). If a file is missing, a simple
    // procedural stand-in is drawn instead.
    Model stageModels_[3];
    GpuMesh fallbackPlant_;

    GpuMesh selectRim_, selectFloor_;
    int hovered_ = -1;
};
