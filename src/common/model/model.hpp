#pragma once

#include <assimp/scene.h>

#include <iostream>
#include <json/json.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <texture/sampler.hpp>
#include <texture/texture2d.hpp>
#include <unordered_map>
#include <vector>

#include "animation/animation.hpp"
#include "animation/bone.hpp"
#include "animation/skeleton.hpp"
#include "components/mesh-renderer.hpp"
#include "material/material.hpp"

namespace our {
    class Model {
        std::string modelDirectory = "";
        std::string name = "";
        std::vector<MeshRendererComponent*> submeshes;  // submeshes with opaque materials (rendered in the first pass)
        Skeleton* skeleton = nullptr;                   // will be nullptr if the model has no animations
        Mesh* combinedMesh = nullptr;  // a single mesh that combines all the submeshes of this model (used for
        // collision detection and other non-rendering purposes)

        void processNode(aiNode* node, const aiScene* scene, glm::mat4& parentTransform,
                         std::vector<SkeletonNode>* skeletonNodes, int parentIndex = -1);
        MeshRendererComponent* processMesh(aiMesh* mesh, const aiScene* scene);
        void processVertexBoneData(std::vector<Vertex>& vertices, aiMesh* mesh);
        void loadMaterialsFromScene(const aiScene* scene);
        void loadAnimationsFromScene(const aiScene* scene);
        LitMaterial* loadMaterial(const aiScene* scene, const aiMaterial* mat);
        Texture2D* loadTextureFromMaterial(const aiScene* scene, const aiMaterial* mat, aiTextureType type);
        void generateCombinedMesh();  // will be used for collision detection and other non-rendering purposes
        void setVertexBoneData(Vertex& vertex, BoneID boneID, float weight);
        void loadAnimationsFromScene(const aiScene* scene, const std::unordered_set<std::string>& animationNames);

    public:
        // maps animation names to their corresponding Animation objects
        std::unordered_map<std::string, Animation> animations;

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

        // this will be used for the progress bar loading
        static int getAssetsCount(const std::string& path, bool countAnimations = true);

        ~Model();

        void loadFromFile(const std::string& path, const std::unordered_set<std::string>& animationNames = {});
    };

}  // namespace our
