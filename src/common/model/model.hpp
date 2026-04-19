#pragma once

#include <assimp/scene.h>

#include <iostream>
#include <json/json.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <texture/sampler.hpp>
#include <texture/texture2d.hpp>
#include <vector>

#include "animation/animation.hpp"
#include "animation/bone.hpp"
#include "animation/skeleton.hpp"
#include "components/mesh-renderer.hpp"
#include "material/material.hpp"

namespace our {
    class Model {
        std::string modelDirectory;
        std::string name;
        std::vector<MeshRendererComponent*> submeshes;  // submeshes with opaque materials (rendered in the first pass)

        void processNode(aiNode* node, const aiScene* scene, glm::mat4& parentTransform);
        MeshRendererComponent* processMesh(aiMesh* mesh, const aiScene* scene);
        void processVertexBoneData(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
        void loadMaterialsFromScene(const aiScene* scene);
        LitMaterial* loadMaterial(const aiScene* scene, const aiMaterial* mat);
        Texture2D* loadTextureFromMaterial(const aiScene* scene, const aiMaterial* mat, aiTextureType type);
        void generateCombinedMesh();  // will be used for collision detection and other non-rendering purposes
        Mesh* combinedMesh;           // a single mesh that combines all the submeshes of this model (used for collision
                                      // detection and other non-rendering purposes)
        void setVertexBoneData(Vertex& vertex, BoneID boneID, float weight);
        Skeleton* skeleton = nullptr;  // will be nullptr if the model has no animations
    public:
        std::unordered_map<std::string, Animation>
            animations;  // maps animation names to their corresponding Animation objects
        Model(const std::string& name) {
            this->name = name;
        };
        std::vector<MeshRendererComponent*>& getSubmeshes() {
            return submeshes;
        }
        Mesh* getCombinedMesh() const {
            return combinedMesh;
        }
        bool hasSkeleton() const {
            return skeleton != nullptr;
        }
        ~Model();
        void loadFromFile(const std::string& path);
    };

}  // namespace our
