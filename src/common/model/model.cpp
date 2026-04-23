#include "model.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <assimp/Importer.hpp>
#include <iostream>
#include <unordered_map>

#include "ai-glm-utils.hpp"
#include "asset-loader.hpp"
#include "texture/texture-utils.hpp"

namespace our {

    void Model::loadFromFile(const std::string& path) {
        std::cout << "Loading model from file: " << path << std::endl;
        Assimp::Importer importer;
        const aiScene* scene =
            importer.ReadFile(path,
                              // Geometry
                              aiProcess_Triangulate |           // convert quads/polygons to triangles
                                  aiProcess_CalcTangentSpace |  // needed for normal mapping
                                  aiProcess_GenSmoothNormals |  // better than GenNormals, uses smoothing angles

                                  // Optimization
                                  aiProcess_JoinIdenticalVertices |  // combine vertices that are identical in position,
                                                                     // normal, tex coords, and color
                                  aiProcess_OptimizeMeshes |         // reduce draw calls
                                  aiProcess_OptimizeGraph |          // optimize node hierarchy
                                  aiProcess_ImproveCacheLocality |   // better GPU cache usage

                                  // Skeletal
                                  aiProcess_LimitBoneWeights |      // limit to 4 weights per vertex (GPU standard)
                                  aiProcess_PopulateArmatureData |  // fills aiBone with proper armature/node data
                                  aiProcess_GlobalScale |  // fixes scale differences between formats (FBX vs GLTF)

                                  // Correctness
                                  aiProcess_GenUVCoords | aiProcess_TransformUVCoords |  // convert to proper UV range
                                  aiProcess_SortByPType);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }

        this->modelDirectory = path.substr(0, path.find_last_of('/'));

        glm::mat4 identity(1.0f);
        loadMaterialsFromScene(scene);

        if (scene->HasAnimations()) {
            skeleton = new Skeleton();
            loadAnimationsFromScene(scene);
            processNode(scene->mRootNode, scene, identity, &skeleton->getNodes());
        } else {
            processNode(scene->mRootNode, scene, identity, nullptr);
        }

        generateCombinedMesh();
    }

    void Model::processNode(aiNode* node, const aiScene* scene, glm::mat4& parentTransform,
                            std::vector<SkeletonNode>* skeletonNodes, int parentIndex) {
        // process all the node's meshes (if any)
        aiMatrix4x4 t = node->mTransformation;
        glm::mat4 nodeTransform = aiToGlm(t);
        glm::mat4 globalTransform = parentTransform * nodeTransform;

        int currentIndex = -1;
        if (skeletonNodes) {
            currentIndex = static_cast<int>(skeletonNodes->size());

            SkeletonNode skeletonNode;
            skeletonNode.name = node->mName.C_Str();
            skeletonNode.localTransform = nodeTransform;
            skeletonNode.parentIndex = parentIndex;
            skeletonNodes->push_back(skeletonNode);
        }

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            MeshRendererComponent* submesh = processMesh(mesh, scene);
            submesh->transform = globalTransform;
            // this is a fix for model that is exported for directX where the coordinate system is left-handed, we can
            // detect that by checking if the transform has a negative scale (which is equivalent to a negative
            // determinant) and if so we flip the winding order of the triangles by applying a negative scale on the X
            // axis and also update the face culling mode to front face = CW instead of CCW
            if (glm::determinant(globalTransform) < 0.0f) {
                submesh->material->pipelineState.faceCulling.frontFace = GL_CW;
                submesh->transform = globalTransform * glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
            }
            submesh->nodeName = node->mName.C_Str();
            submeshes.push_back(submesh);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, globalTransform, skeletonNodes, currentIndex);
        }
    }

    MeshRendererComponent* Model::processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices(mesh->mNumVertices);
        std::vector<unsigned int> indices;
        // process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex v;

            v.position = aiToGlm(mesh->mVertices[i]);

            if (mesh->mNormals) v.normal = aiToGlm(mesh->mNormals[i]);

            auto uv = mesh->mTextureCoords[0] ? &mesh->mTextureCoords[0][i] : nullptr;
            if (uv) v.tex_coord = aiToGlm(*uv);

            aiColor4D* vertexColor = mesh->mColors[0] ? &mesh->mColors[0][i] : nullptr;
            if (vertexColor)
                v.color = our::Color(vertexColor->r, vertexColor->g, vertexColor->b, vertexColor->a);
            else
                v.color = our::Color(255, 255, 255, 255);

            // Tangents and bitangents are needed for normal mapping, but not all models have them. If they don't
            // exist we will set them to zero and the shader will handle it as if the surface is flat (facing up)
            if (mesh->mTangents && mesh->mBitangents) {
                glm::vec3 T = aiToGlm(mesh->mTangents[i]);
                glm::vec3 B = aiToGlm(mesh->mBitangents[i]);
                glm::vec3 N = aiToGlm(mesh->mNormals[i]);

                // Determine handedness sign
                float sign = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
                v.tangent = glm::vec4(T, sign);
            } else {
                v.tangent = glm::vec4(0.0f);
            }

            // bone data
            for (int j = 0; j < MAX_BONE_INFLUENCE; ++j) {
                v.bone_ids[j] = -1;
                v.weights[j] = 0.0f;
            }

            vertices[i] = v;
        }

        if (mesh->HasBones()) processVertexBoneData(vertices, mesh, scene);

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) indices.push_back(face.mIndices[j]);
        }

        our::Mesh* ourMesh = new our::Mesh(vertices, indices);
        std::string name = this->name + "_" + std::to_string(mesh->mMaterialIndex);

        Material* material = AssetLoader<Material>::get(name);
        if (!material) {
            std::cerr << "\033[31mFailed to load material for mesh " << mesh->mName.C_Str() << "\033[0m" << std::endl;
            delete ourMesh;
            return nullptr;
        }

        MeshRendererComponent* meshComponent = new MeshRendererComponent();
        meshComponent->mesh = ourMesh;
        meshComponent->material = material;
        meshComponent->hasBones = mesh->HasBones();
        return meshComponent;
    }

    void Model::loadMaterialsFromScene(const aiScene* scene) {
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* mat = scene->mMaterials[i];
            LitMaterial* material = loadMaterial(scene, mat);
            std::string name = this->name + "_" + std::to_string(i);
            if (material) {
                AssetLoader<Material>::add(name, material);
            }
        }
    }

    void Model::loadAnimationsFromScene(const aiScene* scene) {
        for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
            std::string animName = scene->mAnimations[i]->mName.C_Str();
            if (animName.empty()) {
                animName = "Anim_" + std::to_string(i);
            }
            animations.emplace(animName, Animation(scene->mAnimations[i], *skeleton));
            std::cout << "Loaded animation: \"" << animName << "\"" << std::endl;
        }
    }

    LitMaterial* Model::loadMaterial(const aiScene* scene, const aiMaterial* mat) {
        LitMaterial* material = new LitMaterial();
        material->transparent = false;
        material->metallic = 0.95f;
        material->roughness = 0.1f;

        aiString matName;
        mat->Get(AI_MATKEY_NAME, matName);

        // Setup Shader
        material->shader = AssetLoader<ShaderProgram>::get("lit");
        if (!material->shader) {
            std::cerr << "\033[31mFailed to load shader for material " << matName.C_Str() << "\033[0m" << std::endl;
            delete material;
            material = nullptr;
            return nullptr;
        }

        // albedo factor
        aiColor4D color;
        material->tint = glm::vec4(1.0f);  // default tint is white (no tint)
        if (AI_SUCCESS == mat->Get(AI_MATKEY_BASE_COLOR, color)) {
            material->albedo = aiToGlm(color);
            material->tint.a = color.a;
        } else if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            material->albedo = aiToGlm(color);
            material->tint.a = color.a;
        } else {
            material->albedo = glm::vec3(1.0f, 1.0f, 1.0f);
            material->tint.a = 1.0f;
        }

        // metallic factor
        if (AI_SUCCESS == mat->Get(AI_MATKEY_METALLIC_FACTOR, material->metallic)) {
            material->metallic = std::clamp(material->metallic, 0.0f, 1.0f);
        } else {
            material->metallic = 0.95f;
        }

        // roughness factor
        if (AI_SUCCESS == mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, material->roughness)) {
            material->roughness = std::clamp(material->roughness, 0.0f, 1.0f);
        } else {
            material->roughness = 0.1f;
        }

        // emission factor
        if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
            material->emission = aiToGlm(color);
        }

        // set default ambient occlusion
        material->ambientOcclusion = 1.0f;

        // set textures
        // set base color texture, if not found try diffuse texture
        material->textureAlbedo = loadTextureFromMaterial(scene, mat, aiTextureType_BASE_COLOR);
        if (!material->textureAlbedo) {
            material->textureAlbedo = loadTextureFromMaterial(scene, mat, aiTextureType_DIFFUSE);
        }
        material->mask.hasAlbedo = material->textureAlbedo != nullptr;

        // set normal texture, if not found try height texture
        material->textureNormal = loadTextureFromMaterial(scene, mat, aiTextureType_NORMALS);
        if (!material->textureNormal) {
            material->textureNormal = loadTextureFromMaterial(scene, mat, aiTextureType_HEIGHT);
        }
        material->mask.hasNormal = material->textureNormal != nullptr;

        // set metallic and roughness textures, some models pack them in the same texture so we check for that first
        Texture2D* metallicRoughness = loadTextureFromMaterial(scene, mat, aiTextureType_GLTF_METALLIC_ROUGHNESS);
        if (metallicRoughness) {
            material->textureMetalnessRoughness = metallicRoughness;
            material->mask.hasMetalnessRoughness = true;
        } else {
            material->textureMetallic = loadTextureFromMaterial(scene, mat, aiTextureType_METALNESS);
            material->mask.hasMetallic = material->textureMetallic != nullptr;

            material->textureRoughness = loadTextureFromMaterial(scene, mat, aiTextureType_DIFFUSE_ROUGHNESS);
            material->mask.hasRoughness = material->textureRoughness != nullptr;
        }

        // set ambient occlusion texture, if not found try lightmap texture
        material->textureAmbientOcclusion = loadTextureFromMaterial(scene, mat, aiTextureType_AMBIENT_OCCLUSION);
        if (!material->textureAmbientOcclusion) {
            material->textureAmbientOcclusion = loadTextureFromMaterial(scene, mat, aiTextureType_LIGHTMAP);
        }
        material->mask.hasAmbientOcclusion = material->textureAmbientOcclusion != nullptr;

        // set emissive texture
        material->textureEmissive = loadTextureFromMaterial(scene, mat, aiTextureType_EMISSIVE);
        material->mask.hasEmissive = material->textureEmissive != nullptr;

        // setting pipeline state
        int twoSided;
        if (AI_SUCCESS == mat->Get(AI_MATKEY_TWOSIDED, twoSided)) {
            material->pipelineState.faceCulling.enabled = !twoSided;
            material->pipelineState.faceCulling.culledFace = GL_BACK;
            material->pipelineState.faceCulling.frontFace = GL_CCW;
        }

        // Modulate by opacity
        float opacity = 1.0f;
        if (AI_SUCCESS == mat->Get(AI_MATKEY_OPACITY, opacity)) {
            material->tint.a *= opacity;
        }

        // Modulate by transparency factor
        float transparency = 0.0f;
        if (AI_SUCCESS == mat->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparency)) {
            material->tint.a *= (1.0f - transparency);
        }
        material->transparent = material->tint.a < 0.999f;

        if (material->transparent) {
            material->pipelineState.depthTesting.enabled = GL_TRUE;  // disable depth writing for transparent materials
            material->pipelineState.depthMask = false;
            material->pipelineState.blending.enabled = true;
            material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            int blendFunc = 0;
            if (mat->Get(AI_MATKEY_BLEND_FUNC, blendFunc)) {
                if (blendFunc == aiBlendMode_Additive) {
                    material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
                    material->pipelineState.blending.destinationFactor = GL_ONE;
                } else {
                    material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
                    material->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;
                }
            }
        } else {
            material->pipelineState.depthTesting.enabled = GL_TRUE;  // enable depth writing for opaque materials
            material->pipelineState.depthTesting.function = GL_LEQUAL;
            material->pipelineState.blending.enabled = false;
        }
        return material;
    }

    Texture2D* Model::loadTextureFromMaterial(const aiScene* scene, const aiMaterial* mat, aiTextureType type) {
        aiString path;
        if (mat->GetTextureCount(type) == 0) return nullptr;
        if (mat->GetTexture(type, 0, &path) != AI_SUCCESS) return nullptr;

        Texture2D* texture = nullptr;
        if (path.length > 0 && path.data[0] == '*') {
            // this is an embedded texture, we can load it directly from memory
            const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(path.C_Str());
            if (!embeddedTexture) {
                std::cerr << "Failed to find embedded texture: " << path.C_Str() << std::endl;
                return nullptr;
            }

            if (embeddedTexture->mHeight == 0) {
                texture = texture_utils::loadImageFromMemory(
                    reinterpret_cast<const unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth, true);
            } else {
                // note that this is not handled properly as it assumes the embedded texture is in RGBA format which
                // might not always be the case
                texture = new Texture2D();
                texture->bind();
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, embeddedTexture->mWidth, embeddedTexture->mHeight, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, embeddedTexture->pcData);
                glGenerateMipmap(GL_TEXTURE_2D);
                texture->unbind();
            }
        } else {
            std::string filePath = path.C_Str();
            texture = texture_utils::loadImage(modelDirectory + "/" + filePath, true);
        }

        if (!texture) return nullptr;

        texture->bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        texture->unbind();

        AssetLoader<Texture2D>::add(path.C_Str(), texture);
        return texture;
    }

    void Model::setVertexBoneData(Vertex& vertex, BoneID boneID, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.bone_ids[i] < 0) {
                vertex.bone_ids[i] = boneID;
                vertex.weights[i] = weight;
                return;
            }
        }
        // If we reach here, it means we have more than MAX_BONE_INFLUENCE bones affecting this vertex
        // We will ignore the extra bones and just print a warning
        std::cerr << "Warning: Vertex has more than " << MAX_BONE_INFLUENCE
                  << " bone influences. Extra influences will be ignored." << std::endl;
    }

    void Model::processVertexBoneData(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene) {
        if (!skeleton) return;  // if the model has no animations, we won't have a skeleton to store the bone data in
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            aiBone* bone = mesh->mBones[boneIndex];
            std::string boneName(bone->mName.C_Str());

            BoneID boneID = skeleton->findOrCreateBone(boneName, bone->mOffsetMatrix);

            for (unsigned int j = 0; j < bone->mNumWeights; ++j) {
                auto boneWeight = bone->mWeights[j];
                unsigned int vertexID = boneWeight.mVertexId;
                float weight = boneWeight.mWeight;
                setVertexBoneData(vertices[vertexID], boneID, weight);
            }
        }

        for (Vertex& vertex : vertices) {
            float weightSum = 0.0f;
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                if (vertex.bone_ids[i] >= 0) {
                    weightSum += vertex.weights[i];
                }
            }
            if (weightSum > 0.0f) {
                float invSum = 1.0f / weightSum;
                for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                    if (vertex.bone_ids[i] >= 0) {
                        vertex.weights[i] *= invSum;
                    }
                }
            }
        }
    }

    void Model::generateCombinedMesh() {
        std::vector<Vertex> combinedVertices;
        std::vector<unsigned int> combinedIndices;

        for (const auto& submesh : submeshes) {
            unsigned int indexOffset = static_cast<unsigned int>(combinedVertices.size());

            // Transform each vertex into model space using the submesh's local transform
            for (const Vertex& v : submesh->mesh->getVertices()) {
                Vertex transformed = v;

                // Apply the submesh transform to position
                glm::vec4 worldPos = submesh->transform * glm::vec4(v.position, 1.0f);
                transformed.position = glm::vec3(worldPos);

                // Transform normal using the normal matrix (inverse transpose)
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(submesh->transform)));
                glm::vec3 t = glm::normalize(normalMatrix * glm::vec3(v.tangent));
                transformed.tangent = glm::vec4(t, v.tangent.w);  // keep handedness unchanged

                combinedVertices.push_back(transformed);
            }

            // Re-base indices so they point into the combined vertex buffer
            for (unsigned int idx : submesh->mesh->getIndices()) {
                combinedIndices.push_back(idx + indexOffset);
            }
        }
        combinedMesh = new Mesh(combinedVertices, combinedIndices);
    }

    Model::~Model() {
        for (auto& submesh : submeshes) {
            delete submesh->mesh;
            delete submesh;
        }
        if (skeleton) delete skeleton;

        if (combinedMesh) delete combinedMesh;
        combinedMesh = nullptr;
    }

}  // namespace our
