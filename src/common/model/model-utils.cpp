#include "model-utils.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <asset-loader.hpp>
#include <assimp/Importer.hpp>
#include <iostream>
#include <texture/texture-utils.hpp>

namespace our::model_utils {
    constexpr int MAX_BONE_INFLUENCE = 4;
    Model* loadModel(const std::string& name, const std::string& directory) {
        Assimp::Importer importer;
        const aiScene* scene =
            importer.ReadFile(name, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
                                        aiProcess_GenNormals | aiProcess_LimitBoneWeights);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return nullptr;
        }

        our::Model* model = new our::Model();

        processNode(scene->mRootNode, scene, model->submeshes, directory);

        return model;
    }

    void processNode(aiNode* node, const aiScene* scene, std::vector<our::SubMesh>& submeshes,
                     const std::string& directory) {
        // process all the node's meshes (if any)
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            submeshes.push_back(processMesh(mesh, scene, directory));
        }
        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, submeshes, directory);
        }
    }

    our::SubMesh processMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        // process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
            if (mesh->mTextureCoords[0]) {
                vertex.tex_coord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
            } else {
                vertex.tex_coord = {0.0f, 0.0f};
            }
            vertex.color = {255, 255, 255, 255};
            vertices.push_back(vertex);
        }

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<our::TextureBinding> textures;
        // 1. diffuse maps
        std::vector<our::TextureBinding> diffuseMaps =
            loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", directory);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        std::vector<our::TextureBinding> specularMaps =
            loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", directory);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<our::TextureBinding> normalMaps =
            loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", directory);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<our::TextureBinding> heightMaps =
            loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", directory);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        return our::SubMesh(new our::Mesh(vertices, indices), textures);
    }
    std::vector<our::TextureBinding> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName,
                                                          const std::string& directory) {
        std::vector<our::TextureBinding> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            if (AssetLoader<our::Texture2D>::get(str.C_Str()) != nullptr) {
                textures.push_back({AssetLoader<our::Texture2D>::get(str.C_Str()), typeName});
                continue;
            }
            our::Texture2D* texture = our::texture_utils::loadImage(directory + "/" + str.C_Str(), false);
            AssetLoader<our::Texture2D>::addAsset(str.C_Str(), texture);
            textures.push_back({texture, typeName});
        }
        return textures;
    }

}  // namespace our::model_utils
