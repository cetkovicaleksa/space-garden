// Mesh class from LearnOpenGL (https://learnopengl.com, Joey de Vries, CC BY 4.0).
// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/mesh.h
// Adapted: no tangents/bones; each mesh also keeps its material colors (OBJ Kd/Ks/Ns) and Draw
// sets the Space Garden scene shader uniforms (uMaterial.*, uUseTexture, uTexture).
#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <learnopengl/shader.h>

#include <cstddef>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct MeshMaterial {
    glm::vec3 diffuse{0.8f};
    float specular = 0.2f;
    float shininess = 32.0f;
};

class Mesh {
public:
    // mesh data
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    MeshMaterial              material;
    unsigned int VAO = 0;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures,
         MeshMaterial material)
        : vertices(std::move(vertices)), indices(std::move(indices)), textures(std::move(textures)), material(material)
    {
        setupMesh();
    }

    // render the mesh (the caller sets uModel/uNormalMatrix and the light uniforms)
    void Draw(const Shader& shader) const
    {
        const bool textured = !textures.empty();
        if (textured) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0].id);
        }
        shader.setBool("uUseTexture", textured);
        shader.setVec3("uMaterial.color", material.diffuse);
        shader.setFloat("uMaterial.specular", material.specular);
        shader.setFloat("uMaterial.shininess", material.shininess);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    unsigned int VBO = 0, EBO = 0;

    void setupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};
#endif
