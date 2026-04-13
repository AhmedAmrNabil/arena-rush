#pragma once

#include <assimp/scene.h>

#include <string>
#include <texture/texture2d.hpp>
#include <vector>

#include "model.hpp"

namespace our::model_utils {
    our::Model* loadModel(const std::string& name, const std::string& directory);
    void processNode(aiNode* node, const aiScene* scene, std::vector<our::SubMesh>& submeshes,
                     const std::string& directory);
    our::SubMesh processMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory);
    std::vector<our::TextureBinding> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName,
                                                          const std::string& directory);
}  // namespace our::model_utils
