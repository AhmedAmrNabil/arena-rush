#include "model.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <asset-loader.hpp>
#include <assimp/Importer.hpp>
#include <iostream>

#include "ai-glm-utils.hpp"
#include "texture/texture-utils.hpp"

namespace our {
    void Model::draw(const glm::mat4& VP, const glm::mat4& modelMatrix, const std::vector<our::LightRenderData>& lights,
                     const glm::vec3& cameraPosition) const {
        glm::mat4 MVP = VP * modelMatrix;
        for (const auto& submesh : submeshes) {
            // this dynamic cast will always pass
            // as we only make LitMaterials when loading the model
            if (LitMaterial* litMaterial = dynamic_cast<LitMaterial*>(submesh->material)) {
                litMaterial->setup(lights);
                litMaterial->shader->set("cameraPos", cameraPosition);
                litMaterial->shader->set("model", modelMatrix * submesh->transform);
                litMaterial->shader->set("transform", MVP * submesh->transform);
            } else if (submesh->material) {
                submesh->material->setup();
                submesh->material->shader->set("model", modelMatrix * submesh->transform);
                submesh->material->shader->set("transform", MVP * submesh->transform);
            }
            submesh->mesh->draw();
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
        std::cout << "loaded " << materials.size() << " materials" << std::endl;
        glm::mat4 identity(1.0f);
        processNode(scene->mRootNode, scene, identity);
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

        Material* material = nullptr;
        if (mesh->mMaterialIndex >= 0 && static_cast<size_t>(mesh->mMaterialIndex) < materials.size()) {
            material = materials[mesh->mMaterialIndex];
        } else {
            std::cerr << "Warning: mesh has invalid material index " << mesh->mMaterialIndex << std::endl;
        }

        MeshRendererComponent* meshComponent = new MeshRendererComponent();
        meshComponent->mesh = ourMesh;
        meshComponent->material = material;
        return meshComponent;
    }

    void Model::loadMaterialsFromScene(const aiScene* scene) {
        materials.clear();
        materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* mat = scene->mMaterials[i];
            materials.push_back(loadMaterial(mat));
        }
    }

    LitMaterial* Model::loadMaterial(const aiMaterial* mat) {
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
        material->textureAlbedo = loadTextureFromMaterial(mat, aiTextureType_BASE_COLOR);
        if (!material->textureAlbedo) {
            material->textureAlbedo = loadTextureFromMaterial(mat, aiTextureType_DIFFUSE);
        }
        material->mask.hasAlbedo = material->textureAlbedo != nullptr;

        // set normal texture, if not found try height texture
        material->textureNormal = loadTextureFromMaterial(mat, aiTextureType_NORMALS);
        if (!material->textureNormal) {
            material->textureNormal = loadTextureFromMaterial(mat, aiTextureType_HEIGHT);
        }
        material->mask.hasNormal = material->textureNormal != nullptr;

        // set metallic and roughness textures, some models pack them in the same texture so we check for that first
        Texture2D* metallicRoughness = loadTextureFromMaterial(mat, aiTextureType_GLTF_METALLIC_ROUGHNESS);
        if (metallicRoughness) {
            material->textureMetalnessRoughness = metallicRoughness;
            material->mask.hasMetalnessRoughness = true;
        } else {
            material->textureMetallic = loadTextureFromMaterial(mat, aiTextureType_METALNESS);
            material->mask.hasMetallic = material->textureMetallic != nullptr;

            material->textureRoughness = loadTextureFromMaterial(mat, aiTextureType_DIFFUSE_ROUGHNESS);
            material->mask.hasRoughness = material->textureRoughness != nullptr;
        }

        // set ambient occlusion texture, if not found try lightmap texture
        material->textureAmbientOcclusion = loadTextureFromMaterial(mat, aiTextureType_AMBIENT_OCCLUSION);
        if (!material->textureAmbientOcclusion) {
            material->textureAmbientOcclusion = loadTextureFromMaterial(mat, aiTextureType_LIGHTMAP);
        }
        material->mask.hasAmbientOcclusion = material->textureAmbientOcclusion != nullptr;

        // set emissive texture
        material->textureEmissive = loadTextureFromMaterial(mat, aiTextureType_EMISSIVE);
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
            material->pipelineState.depthTesting.enabled = GL_FALSE;  // disable depth writing for transparent materials
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
        material->print();
        return material;
    }

    Texture2D* Model::loadTextureFromMaterial(const aiMaterial* mat, aiTextureType type) {
        aiString path;
        if (mat->GetTextureCount(type) == 0) return nullptr;
        if (mat->GetTexture(type, 0, &path) != AI_SUCCESS) return nullptr;

        std::string filePath = path.C_Str();
        Texture2D* texture = texture_utils::loadImage(modelDirectory + "/" + filePath, type);
        std::cout << "Loaded texture: " << filePath << std::endl;
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

    Model::~Model() {
        for (auto& submesh : submeshes) {
            delete submesh->mesh;
            delete submesh;
        }
        for (auto& material : materials) {
            delete material;
        }
        for (auto& pair : loadedTextures) {
            delete pair.second;
        }
    }

}  // namespace our
