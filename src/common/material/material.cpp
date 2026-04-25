#include "material.hpp"

#include <iostream>

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        pipelineState.setup();
        if (shader)
            shader->use();
        else
            std::cerr << "Warning: trying to use a material with no shader!" << std::endl;
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        if (data.contains("pipelineState")) {
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint
    void TintedMaterial::setup() const {
        Material::setup();
        shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json& data) {
        Material::deserialize(data);
        if (!data.is_object()) return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable
    // "tex"
    void TexturedMaterial::setup() const {
        TintedMaterial::setup();
        shader->set("alphaThreshold", alphaThreshold);
        shader->set("uvScale", uvScale);
        glActiveTexture(GL_TEXTURE0);
        if (texture) {
            texture->bind();
        }
        if (sampler) {
            sampler->bind(0);
        }
        shader->set("tex", 0);
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data) {
        TintedMaterial::deserialize(data);
        if (!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
        uvScale = data.value("uvScale", uvScale);
    }

    void LitMaterial::setLightUniforms(const std::vector<our::LightRenderData>& lights) const {
        auto lightCount = std::min(lights.size(), MAX_LIGHTS);
        shader->set("numLights", lightCount);
        for (size_t i = 0; i < lightCount; i++) {
            const auto& light = lights[i];
            std::string prefix = "lights[" + std::to_string(i) + "]";
            shader->set(prefix + ".type", static_cast<int>(light.type));
            shader->set(prefix + ".color", light.color);
            shader->set(prefix + ".position", light.position);
            if (light.type == LightType::DIRECTIONAL) {
                shader->set(prefix + ".direction", light.direction);
            }
            if (light.type == LightType::POINT) {
                shader->set(prefix + ".attenuation", light.attenuation);
            }
            if (light.type == LightType::SPOT) {
                shader->set(prefix + ".attenuation", light.attenuation);
                shader->set(prefix + ".direction", light.direction);
                shader->set(prefix + ".spotAngles", light.spotAngles);
            }
        }
    }

    void LitMaterial::setup() const {
        TintedMaterial::setup();

        shader->set("alphaThreshold", alphaThreshold);
        shader->set("material.albedo", albedo);
        shader->set("material.metallic", metallic);
        shader->set("material.roughness", roughness);
        shader->set("material.ambientOcclusion", ambientOcclusion);
        shader->set("material.emission", emission);

        shader->set("material.hasTextureAlbedo", mask.hasAlbedo);
        if (mask.hasAlbedo) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::ALBEDO));
            textureAlbedo->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::ALBEDO));
            }
            shader->set("material.textureAlbedo", static_cast<int>(TextureUnits::ALBEDO));
        }

        shader->set("material.hasTextureMetallic", mask.hasMetallic);
        if (mask.hasMetallic) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::METALLIC));
            textureMetallic->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::METALLIC));
            }
            shader->set("material.textureMetallic", static_cast<int>(TextureUnits::METALLIC));
        }

        shader->set("material.hasTextureRoughness", mask.hasRoughness);
        if (mask.hasRoughness) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::ROUGHNESS));
            textureRoughness->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::ROUGHNESS));
            }
            shader->set("material.textureRoughness", static_cast<int>(TextureUnits::ROUGHNESS));
        }

        shader->set("material.hasTextureNormal", mask.hasNormal);
        if (mask.hasNormal) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::NORMAL));
            textureNormal->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::NORMAL));
            }
            shader->set("material.textureNormal", static_cast<int>(TextureUnits::NORMAL));
        }

        shader->set("material.hasTextureAmbientOcclusion", mask.hasAmbientOcclusion);
        if (mask.hasAmbientOcclusion) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
            textureAmbientOcclusion->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
            }
            shader->set("material.textureAmbientOcclusion", static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
        }

        shader->set("material.hasTextureEmissive", mask.hasEmissive);
        if (mask.hasEmissive) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::EMISSIVE));
            textureEmissive->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::EMISSIVE));
            }
            shader->set("material.textureEmissive", static_cast<int>(TextureUnits::EMISSIVE));
        }
        shader->set("material.hasMetalnessRoughness", mask.hasMetalnessRoughness);
        if (mask.hasMetalnessRoughness) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::METALLIC_ROUGHNESS));
            textureMetalnessRoughness->bind();
            if (sampler) {
                sampler->bind(static_cast<int>(TextureUnits::METALLIC_ROUGHNESS));
            }
            shader->set("material.textureMetalnessRoughness", static_cast<int>(TextureUnits::METALLIC_ROUGHNESS));
        }
    }

    void LitMaterial::setup(const std::vector<our::LightRenderData>& lights) const {
        setup();
        setLightUniforms(lights);
    }

    void LitMaterial::deserialize(const nlohmann::json& data) {
        TintedMaterial::deserialize(data);
        if (!data.is_object()) return;
        sampler = AssetLoader<Sampler>::get(data.value("sampler", "default"));
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        albedo = data.value("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
        metallic = data.value("metallic", 0.2f);
        roughness = data.value("roughness", 0.2f);
        ambientOcclusion = data.value("ambientOcclusion", 1.0f);
        emission = data.value("emission", glm::vec3(0.0f, 0.0f, 0.0f));

        textureAlbedo = AssetLoader<Texture2D>::get(data.value("textureAlbedo", ""));
        mask.hasAlbedo = textureAlbedo != nullptr;

        textureMetallic = AssetLoader<Texture2D>::get(data.value("textureMetallic", ""));
        mask.hasMetallic = textureMetallic != nullptr;

        textureRoughness = AssetLoader<Texture2D>::get(data.value("textureRoughness", ""));
        mask.hasRoughness = textureRoughness != nullptr;

        textureNormal = AssetLoader<Texture2D>::get(data.value("textureNormal", ""));
        mask.hasNormal = textureNormal != nullptr;

        textureAmbientOcclusion = AssetLoader<Texture2D>::get(data.value("textureAmbientOcclusion", ""));
        mask.hasAmbientOcclusion = textureAmbientOcclusion != nullptr;

        textureEmissive = AssetLoader<Texture2D>::get(data.value("textureEmissive", ""));
        mask.hasEmissive = textureEmissive != nullptr;

        textureMetalnessRoughness = AssetLoader<Texture2D>::get(data.value("textureMetalnessRoughness", ""));
        mask.hasMetalnessRoughness = textureMetalnessRoughness != nullptr;
    }

}  // namespace our
