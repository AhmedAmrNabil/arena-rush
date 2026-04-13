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
        glm::mat4 identity(1.0f);

        processNode(scene->mRootNode, scene, model->submeshes, directory, identity);

        return model;
    }

    void processNode(aiNode* node, const aiScene* scene, std::vector<our::SubMesh>& submeshes,
                     const std::string& directory, glm::mat4& parentTransform) {
        // process all the node's meshes (if any)
        aiMatrix4x4 t = node->mTransformation;
        // clang-format off
        glm::mat4 nodeTransform = glm::mat4(
            t.a1, t.b1, t.c1, t.d1,
            t.a2, t.b2, t.c2, t.d2,
            t.a3, t.b3, t.c3, t.d3,
            t.a4, t.b4, t.c4, t.d4
        );
        // clang-format on
        glm::mat4 globalTransform = parentTransform * nodeTransform;
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            our::SubMesh submesh = processMesh(mesh, scene, directory);
            submesh.transform = globalTransform;
            submeshes.push_back(submesh);
        }
        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, submeshes, directory, globalTransform);
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
            vertex.color = {255, 255, 255, 255};  // default white color
            vertices.push_back(vertex);
        }

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D diffuse, specular, ambient, emissive;
        float shininess;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        material->Get(AI_MATKEY_SHININESS, shininess);

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

        return our::SubMesh(new our::Mesh(vertices, indices), textures,
                            {{diffuse.r, diffuse.g, diffuse.b, diffuse.a},
                             {ambient.r, ambient.g, ambient.b, ambient.a},
                             {specular.r, specular.g, specular.b, specular.a},
                             {emissive.r, emissive.g, emissive.b, emissive.a},
                             shininess},
                            glm::mat4(1.0f));
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
