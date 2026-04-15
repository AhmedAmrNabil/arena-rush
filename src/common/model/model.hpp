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

namespace our {

    class Model {
        static int ID_COUNTER;  // this is used to give each model a unique ID for caching purposes
        int id;                 // the unique ID of this model
        std::string modelDirectory;
        std::vector<MeshRendererComponent*> submeshes;

        void processNode(aiNode* node, const aiScene* scene, glm::mat4& parentTransform);
        MeshRendererComponent* processMesh(aiMesh* mesh, const aiScene* scene);
        void processVertexBoneData(aiMesh* mesh, std::vector<Vertex>& vertices);
        void loadMaterialsFromScene(const aiScene* scene);
        LitMaterial* loadMaterial(const aiScene* scene, const aiMaterial* mat);
        Texture2D* loadTextureFromMaterial(const aiScene* scene, const aiMaterial* mat, aiTextureType type);

    public:
        Model() {
            id = ID_COUNTER++;
        };
        void draw(const glm::mat4& VP, const glm::mat4& modelMatrix, const std::vector<our::LightRenderData>& lights,
                  const glm::vec3& cameraPosition) const;
        ~Model();
        void loadFromFile(const std::string& path);
    };

}  // namespace our
