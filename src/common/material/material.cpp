#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        pipelineState.setup();
        shader->use();
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
    }

    void LitMaterial::setLightUniforms(const std::vector<our::Light*>& lights) const {
        shader->set("numLights", static_cast<int>(lights.size()));
        for (size_t i = 0; i < lights.size(); i++) {
            const auto& light = lights[i];
            std::string prefix = "lights[" + std::to_string(i) + "]";
            shader->set(prefix + ".type", static_cast<int>(light->type));
            shader->set(prefix + ".color", light->color);
            shader->set(prefix + ".position", light->position);
            if (light->type == LightType::DIRECTIONAL) {
                shader->set(prefix + ".direction", light->direction);
            }
            if (light->type == LightType::POINT) {
                shader->set(prefix + ".attenuation", light->attenuation);
            }
            if (light->type == LightType::SPOT) {
                shader->set(prefix + ".attenuation", light->attenuation);
                shader->set(prefix + ".direction", light->direction);
                shader->set(prefix + ".spotAngles", light->spotAngles);
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

        shader->set("material.hasTextureAlbedo", textureAlbedo != nullptr);
        if (textureAlbedo) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::ALBEDO));
            textureAlbedo->bind();
            shader->set("material.textureAlbedo", static_cast<int>(TextureUnits::ALBEDO));
        }

        shader->set("material.hasTextureMetallic", textureMetallic != nullptr);
        if (textureMetallic) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::METALLIC));
            textureMetallic->bind();
            shader->set("material.textureMetallic", static_cast<int>(TextureUnits::METALLIC));
        }

        shader->set("material.hasTextureRoughness", textureRoughness != nullptr);
        if (textureRoughness) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::ROUGHNESS));
            textureRoughness->bind();
            shader->set("material.textureRoughness", static_cast<int>(TextureUnits::ROUGHNESS));
        }

        shader->set("material.hasTextureNormal", textureNormal != nullptr);
        if (textureNormal) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::NORMAL));
            textureNormal->bind();
            shader->set("material.textureNormal", static_cast<int>(TextureUnits::NORMAL));
        }

        shader->set("material.hasTextureAmbientOcclusion", textureAmbientOcclusion != nullptr);
        if (textureAmbientOcclusion) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
            textureAmbientOcclusion->bind();
            shader->set("material.textureAmbientOcclusion", static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
        }

        shader->set("material.hasTextureEmissive", textureEmissive != nullptr);
        if (textureEmissive) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::EMISSIVE));
            textureEmissive->bind();
            shader->set("material.textureEmissive", static_cast<int>(TextureUnits::EMISSIVE));
        }
    }

    void LitMaterial::setup(std::vector<our::Light*>& lights) const {
        TintedMaterial::setup();
        setLightUniforms(lights);

        shader->set("alphaThreshold", alphaThreshold);
        shader->set("material.albedo", albedo);
        shader->set("material.metallic", metallic);
        shader->set("material.roughness", roughness);
        shader->set("material.ambientOcclusion", ambientOcclusion);
        shader->set("material.emission", emission);

        shader->set("material.hasTextureAlbedo", textureAlbedo != nullptr);
        if (textureAlbedo) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::ALBEDO));
            textureAlbedo->bind();
            shader->set("material.textureAlbedo", static_cast<int>(TextureUnits::ALBEDO));
        }

        shader->set("material.hasTextureMetallic", textureMetallic != nullptr);
        if (textureMetallic) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::METALLIC));
            textureMetallic->bind();
            shader->set("material.textureMetallic", static_cast<int>(TextureUnits::METALLIC));
        }

        shader->set("material.hasTextureRoughness", textureRoughness != nullptr);
        if (textureRoughness) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::ROUGHNESS));
            textureRoughness->bind();
            shader->set("material.textureRoughness", static_cast<int>(TextureUnits::ROUGHNESS));
        }

        shader->set("material.hasTextureNormal", textureNormal != nullptr);
        if (textureNormal) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::NORMAL));
            textureNormal->bind();
            shader->set("material.textureNormal", static_cast<int>(TextureUnits::NORMAL));
        }

        shader->set("material.hasTextureAmbientOcclusion", textureAmbientOcclusion != nullptr);
        if (textureAmbientOcclusion) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
            textureAmbientOcclusion->bind();
            shader->set("material.textureAmbientOcclusion", static_cast<int>(TextureUnits::AMBIENT_OCCLUSION));
        }

        shader->set("material.hasTextureEmissive", textureEmissive != nullptr);
        if (textureEmissive) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(TextureUnits::EMISSIVE));
            textureEmissive->bind();
            shader->set("material.textureEmissive", static_cast<int>(TextureUnits::EMISSIVE));
        }
    }

    void LitMaterial::deserialize(const nlohmann::json& data) {
        TintedMaterial::deserialize(data);
        if (!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        albedo = data.value("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
        metallic = data.value("metallic", 0.2f);
        roughness = data.value("roughness", 0.2f);
        ambientOcclusion = data.value("ambientOcclusion", 1.0f);
        emission = data.value("emission", glm::vec3(0.0f, 0.0f, 0.0f));
        textureAlbedo = AssetLoader<Texture2D>::get(data.value("textureAlbedo", nullptr));
        textureMetallic = AssetLoader<Texture2D>::get(data.value("textureMetallic", nullptr));
        textureRoughness = AssetLoader<Texture2D>::get(data.value("textureRoughness", nullptr));
        textureNormal = AssetLoader<Texture2D>::get(data.value("textureNormal", nullptr));
        textureAmbientOcclusion = AssetLoader<Texture2D>::get(data.value("textureAmbientOcclusion", nullptr));
        textureEmissive = AssetLoader<Texture2D>::get(data.value("textureEmissive", nullptr));
    }

}  // namespace our
