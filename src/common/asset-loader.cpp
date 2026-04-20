#include "asset-loader.hpp"

#include "audio/audio-buffer.hpp"
#include "audio/audio-utils.hpp"
#include "deserialize-utils.hpp"
#include "logger.hpp"
#include "material/material.hpp"
#include "mesh/mesh-utils.hpp"
#include "mesh/mesh.hpp"
#include "model/model.hpp"
#include "shader/shader.hpp"
#include "texture/sampler.hpp"
#include "texture/texture-utils.hpp"
#include "texture/texture2d.hpp"

namespace our {

    // This will load all the shaders defined in "data"
    // data must be in the form:
    //    { shader_name : { "vs" : "path/to/vertex-shader", "fs" : "path/to/fragment-shader" }, ... }
    template <>
    void AssetLoader<ShaderProgram>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                std::string vsPath = desc.value("vs", "");
                std::string fsPath = desc.value("fs", "");
                auto shader = new ShaderProgram();
                shader->attach(vsPath, GL_VERTEX_SHADER);
                shader->attach(fsPath, GL_FRAGMENT_SHADER);
                shader->link();
                assets[name] = shader;
                LOG_DEBUG("AssetLoader", "Loaded shader \"%s\" with vs: \"%s\" and fs: \"%s\"", name.c_str(),
                          vsPath.c_str(), fsPath.c_str());
            }
        }
    };

    // This will load all the textures defined in "data"
    // data must be in the form:
    //    { texture_name : "path/to/image", ... }
    template <>
    void AssetLoader<Texture2D>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                std::string path = desc.get<std::string>();
                assets[name] = texture_utils::loadImage(path);
                LOG_DEBUG("AssetLoader", "Loaded texture \"%s\" from path: \"%s\"", name.c_str(), path.c_str());
            }
        }
    };

    // This will load all the samplers defined in "data"
    // data must be in the form:
    //    { sampler_name : parameters, ... }
    // Where parameters is an object where:
    //      The key is the parameter name, e.g. "MAG_FILTER", "MIN_FILTER", "WRAP_S", "WRAP_T" or "MAX_ANISOTROPY"
    //      The value is the parameter value, e.g. "GL_NEAREST", "GL_REPEAT"
    //  For "MAX_ANISOTROPY", the value must be a float with a value >= 1.0f
    template <>
    void AssetLoader<Sampler>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                auto sampler = new Sampler();
                sampler->deserialize(desc);
                assets[name] = sampler;
                LOG_DEBUG("AssetLoader", "Loaded sampler \"%s\"", name.c_str());
            }
        }
    };

    // This will load all the meshes defined in "data"
    // data must be in the form:
    //    { mesh_name : "path/to/3d-model-file", ... }
    template <>
    void AssetLoader<Mesh>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                std::string path = desc.get<std::string>();
                assets[name] = mesh_utils::loadOBJ(path);
                LOG_DEBUG("AssetLoader", "Loaded mesh \"%s\" from path: \"%s\"", name.c_str(), path.c_str());
            }
        }
    };

    // This will load all the materials defined in "data"
    // Material deserialization depends on shaders, textures and samplers
    // so you must deserialize these 3 asset types before deserializing materials
    // data must be in the form:
    //    { material_name : parameters, ... }
    // Where parameters is an object where the keys can be:
    //      "type" where the value is a string defining the type of the material.
    //              the type will decide which class will be instanced in the function "createMaterialFromType" found in
    //              "material.hpp"
    //      "shader" where the value must be the name of a loaded shader
    //      "pipelineState" (optional) where the value is a json object that can be read by "PipelineState::deserialize"
    //      "transparent" (optional, default=false) where the value is a boolean indicating whether the material is
    //      transparent or not
    //      ... more keys/values can be added depending on the material type (e.g. "texture", "sampler", "tint")
    template <>
    void AssetLoader<Material>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                std::string type = desc.value("type", "");
                auto material = createMaterialFromType(type);
                material->deserialize(desc);
                assets[name] = material;
                LOG_DEBUG("AssetLoader", "Loaded material \"%s\" of type \"%s\"", name.c_str(), type.c_str());
            }
        }
    };

    // This will load all the audio buffers defined in "data"
    // data must be in the form:
    //    { sound_name : "path/to/audio-file", ... }
    template <>
    void AssetLoader<AudioBuffer>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                std::string path = desc.get<std::string>();
                AudioBuffer* buffer = audio_utils::loadWAV(path);
                if (buffer) assets[name] = buffer;
                LOG_DEBUG("AssetLoader", "Loaded audio buffer \"%s\" from path: \"%s\"", name.c_str(), path.c_str());
            }
        }
    }

    template <>
    void AssetLoader<our::Light>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                our::Light* light = new our::Light();
                light->deserialize(desc);
                assets[name] = light;
                LOG_DEBUG("AssetLoader", "Loaded light \"%s\"", name.c_str());
            }
        }
    }

    template <>
    void AssetLoader<our::Model>::deserialize(const nlohmann::json& data) {
        if (data.is_object()) {
            for (auto& [name, desc] : data.items()) {
                auto model = new our::Model(name);
                if (desc.is_string()) {
                    model->loadFromFile(desc.get<std::string>());
                } else if (desc.is_object()) {
                    const std::string path = desc.at("path").get<std::string>();
                    std::unordered_set<std::string> animationNames;
                    if (desc.contains("animations")) {
                        for (const auto& anim : desc["animations"]) {
                            animationNames.insert(anim.get<std::string>());
                        }
                        model->loadFromFile(path, animationNames);
                    } else {
                        model->loadFromFile(path);
                    }
                }

                assets[name] = model;
                LOG_DEBUG("AssetLoader", "Loaded model \"%s\" from path: \"%s\"", name.c_str(),
                          desc.get<std::string>().c_str());
            }
        }
    };

    void deserializeAllAssets(const nlohmann::json& assetData) {
        if (!assetData.is_object()) return;
        if (assetData.contains("shaders")) AssetLoader<ShaderProgram>::deserialize(assetData["shaders"]);
        if (assetData.contains("textures")) AssetLoader<Texture2D>::deserialize(assetData["textures"]);
        if (assetData.contains("samplers")) AssetLoader<Sampler>::deserialize(assetData["samplers"]);
        if (assetData.contains("meshes")) AssetLoader<Mesh>::deserialize(assetData["meshes"]);
        if (assetData.contains("materials")) AssetLoader<Material>::deserialize(assetData["materials"]);
        if (assetData.contains("sounds")) AssetLoader<AudioBuffer>::deserialize(assetData["sounds"]);
        if (assetData.contains("lights")) AssetLoader<our::Light>::deserialize(assetData["lights"]);
        if (assetData.contains("models")) AssetLoader<our::Model>::deserialize(assetData["models"]);
        // setting some default assets if something is missing
        if (!AssetLoader<Sampler>::get("default")) {
            Sampler* defaultSampler = new Sampler();
            AssetLoader<Sampler>::add("default", defaultSampler);
        }
    }

    void clearAllAssets() {
        AssetLoader<ShaderProgram>::clear();
        AssetLoader<Texture2D>::clear();
        AssetLoader<Sampler>::clear();
        AssetLoader<Mesh>::clear();
        AssetLoader<Material>::clear();
        AssetLoader<AudioBuffer>::clear();
        AssetLoader<our::Model>::clear();
    }

}  // namespace our
