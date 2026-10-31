// Model class from LearnOpenGL (https://learnopengl.com, Joey de Vries, CC BY 4.0).
// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/model.h
// Adapted: loads only what the plant models need (positions, normals, uvs, diffuse texture and
// material colors), throws instead of printing on load errors, no gamma option.
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <stdexcept>
#include <string>
#include <vector>

inline unsigned int TextureFromFile(const char* path, const std::string& directory);

class Model {
public:
    // model data
    std::vector<Texture> textures_loaded; // stores all the textures loaded so far, so they are loaded only once
    std::vector<Mesh>    meshes;
    std::string          directory;

    Model() = default;
    // constructor, expects a filepath to a 3D model.
    explicit Model(const std::string& path) { loadModel(path); }

    // draws the model, and thus all its meshes
    void Draw(const Shader& shader) const
    {
        for (const Mesh& mesh : meshes)
            mesh.Draw(shader);
    }

    bool empty() const { return meshes.empty(); }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                           aiProcess_JoinIdenticalVertices);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            throw std::runtime_error("ASSIMP: " + std::string(importer.GetErrorString()));
        directory = path.substr(0, path.find_last_of("/\\"));
        processNode(scene->mRootNode, scene);
    }

    // processes a node recursively: each mesh of the node, then its children.
    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertex.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
                                               : glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.TexCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                                                       : glm::vec2(0.0f);
            vertices.push_back(vertex);
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        MeshMaterial colors;
        aiColor3D kd(0.8f, 0.8f, 0.8f), ks(0.2f, 0.2f, 0.2f);
        float ns = 32.0f;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, kd);
        material->Get(AI_MATKEY_COLOR_SPECULAR, ks);
        material->Get(AI_MATKEY_SHININESS, ns);
        colors.diffuse = glm::vec3(kd.r, kd.g, kd.b);
        colors.specular = (ks.r + ks.g + ks.b) / 3.0f;
        colors.shininess = ns > 0.0f ? ns : 32.0f;

        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        return Mesh(vertices, indices, textures, colors);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for (const Texture& loaded : textures_loaded) {
                if (loaded.path == str.C_Str()) {
                    textures.push_back(loaded);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

inline unsigned int TextureFromFile(const char* path, const std::string& directory)
{
    const std::string filename = directory + '/' + path;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (!data)
        throw std::runtime_error("Texture failed to load at path: " + filename);

    GLenum format = nrComponents == 1 ? GL_RED : nrComponents == 3 ? GL_RGB : GL_RGBA;
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    return textureID;
}
#endif
