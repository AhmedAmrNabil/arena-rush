#include "model.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <asset-loader.hpp>
#include <assimp/Importer.hpp>
#include <iostream>

#include "ai-glm-utils.hpp"
#include "texture/texture-utils.hpp"

namespace our {
    int Model::ID_COUNTER = 0;

    void Model::generateDrawCommands(std::vector<RenderCommand>& modelCommands,
                                     std::vector<RenderCommand>& transparentCommands,
                                     const glm::mat4& modelMatrix) const {
        for (const auto& submesh : submeshes) {
            RenderCommand cmd;
            cmd.localToWorld = modelMatrix * submesh->transform;
            cmd.center = glm::vec3(cmd.localToWorld * glm::vec4(0, 0, 0, 1));
            cmd.mesh = submesh->mesh;
            cmd.material = submesh->material;
            if (submesh->material->transparent) {
                transparentCommands.push_back(cmd);
            } else {
                modelCommands.push_back(cmd);
            }
        }
    }

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
                                  aiProcess_JoinIdenticalVertices |  // reduce vertex count
                                  aiProcess_OptimizeMeshes |         // reduce draw calls
                                  aiProcess_ImproveCacheLocality |   // better GPU cache usage

                                  // Skeletal
                                  aiProcess_LimitBoneWeights |      // limit to 4 weights per vertex (GPU standard)
                                  aiProcess_PopulateArmatureData |  // fills aiBone with proper armature/node data
                                  aiProcess_GlobalScale |  // fixes scale differences between formats (FBX vs GLTF)

                                  // Correctness
                                  aiProcess_ValidateDataStructure |                      // debug: catch malformed files
                                  aiProcess_FindInvalidData |                            // remove degenerate geometry
                                  aiProcess_GenUVCoords | aiProcess_TransformUVCoords |  // convert to proper UV range
                                  aiProcess_SortByPType);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }

        this->modelDirectory = path.substr(0, path.find_last_of('/'));

        loadMaterialsFromScene(scene);
        glm::mat4 identity(1.0f);
        processNode(scene->mRootNode, scene, identity);
        generateCombinedMesh();
    }

    void Model::processNode(aiNode* node, const aiScene* scene, glm::mat4& parentTransform) {
        // process all the node's meshes (if any)
        aiMatrix4x4 t = node->mTransformation;
        glm::mat4 nodeTransform = aiToGlm(t);
        glm::mat4 globalTransform = parentTransform * nodeTransform;
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            MeshRendererComponent* submesh = processMesh(mesh, scene);
            submesh->transform = globalTransform;
            submeshes.push_back(submesh);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, globalTransform);
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

            // bone data
            for (int j = 0; j < MAX_BONE_INFLUENCE; ++j) {
                v.bone_ids[j] = -1;
                v.weights[j] = 0.0f;
            }

            vertices[i] = v;
        }

        processVertexBoneData(mesh, vertices);

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) indices.push_back(face.mIndices[j]);
        }

        our::Mesh* ourMesh = new our::Mesh(vertices, indices);
        std::string name = std::to_string(id) + "_" + std::to_string(mesh->mMaterialIndex);

        Material* material = AssetLoader<Material>::get(name);
        if (!material) {
            std::cerr << "\033[31mFailed to load material for mesh " << mesh->mName.C_Str() << "\033[0m" << std::endl;
            material = new Material();  // fallback to default material
        }

        MeshRendererComponent* meshComponent = new MeshRendererComponent();
        meshComponent->mesh = ourMesh;
        meshComponent->material = material;
        return meshComponent;
    }

    void Model::loadMaterialsFromScene(const aiScene* scene) {
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* mat = scene->mMaterials[i];
            LitMaterial* material = loadMaterial(scene, mat);
            std::string name = std::to_string(id) + "_" + std::to_string(i);
            if (material) {
                AssetLoader<Material>::add(name, material);
            }
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

    struct BoneInfo {
        glm::mat4 offsetMatrix;
    };

    void Model::processVertexBoneData(aiMesh* mesh, std::vector<Vertex>& vertices) {
        static std::unordered_map<std::string, int> boneMapping(0);  // maps a bone name to its index
        std::vector<BoneInfo> boneInfo;
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            aiBone* bone = mesh->mBones[i];
            std::string boneName(bone->mName.C_Str());
            int boneID;
            if (boneMapping.find(boneName) == boneMapping.end()) {
                boneID = boneMapping.size();
                boneMapping[boneName] = boneID;
                boneInfo.push_back({aiToGlm(bone->mOffsetMatrix)});
            } else {
                boneID = boneMapping[boneName];
            }

            for (unsigned int j = 0; j < bone->mNumWeights; ++j) {
                aiVertexWeight weight = bone->mWeights[j];
                unsigned int vertexId = weight.mVertexId;
                float weightValue = weight.mWeight;

                for (int k = 0; k < MAX_BONE_INFLUENCE; ++k) {
                    if (vertices[vertexId].bone_ids[k] == -1) {
                        vertices[vertexId].bone_ids[k] = boneID;
                        vertices[vertexId].weights[k] = weightValue;
                        break;
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
                transformed.normal = glm::normalize(normalMatrix * v.normal);

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
    }

}  // namespace our
