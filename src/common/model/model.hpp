#pragma once

#include <assimp/scene.h>

#include <iostream>
#include <json/json.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <texture/sampler.hpp>
#include <texture/texture2d.hpp>
#include <vector>

#include "components/mesh-renderer.hpp"
#include "material/material.hpp"
#include "systems/render-command.hpp"

namespace our {
    class Model {
        std::string modelDirectory;
        std::string name;
        std::vector<MeshRendererComponent*> submeshes;  // submeshes with opaque materials (rendered in the first pass)

        void processNode(aiNode* node, const aiScene* scene, glm::mat4& parentTransform);
        MeshRendererComponent* processMesh(aiMesh* mesh, const aiScene* scene);
        void processVertexBoneData(aiMesh* mesh, std::vector<Vertex>& vertices);
        void loadMaterialsFromScene(const aiScene* scene);
        LitMaterial* loadMaterial(const aiScene* scene, const aiMaterial* mat);
        Texture2D* loadTextureFromMaterial(const aiScene* scene, const aiMaterial* mat, aiTextureType type);
        void generateCombinedMesh();  // will be used for collision detection and other non-rendering purposes
        Mesh* combinedMesh;           // a single mesh that combines all the submeshes of this model (used for collision
                                      // detection and other non-rendering purposes)

    public:
        Model(const std::string& name) {
            this->name = name;
        };
        void generateDrawCommands(std::vector<RenderCommand>& modelCommands,
                                  std::vector<RenderCommand>& transparentCommands, const glm::mat4& modelMatrix) const;
        Mesh* getCombinedMesh() const {
            return combinedMesh;
        }
        ~Model();
        void loadFromFile(const std::string& path);
    };

}  // namespace our
