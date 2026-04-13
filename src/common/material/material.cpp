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
        shader = AssetLoader<ShaderProgram>::get(data.at("shader").get<std::string>());
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

    void ModelMaterial::deserialize(const nlohmann::json& data) {
        TintedMaterial::deserialize(data);
        if (!data.is_object()) return;
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data) {
        ModelMaterial::deserialize(data);
        if (!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
    }

    void ModelMaterial::setup() const {
        TintedMaterial::setup();
        // we will bind the textures in the model's submeshes to texture units 0-5
        // so we just need to bind the sampler to these texture units
        for (int i = 0; i < 6; i++) {
            if (sampler) {
                sampler->bind(i);
            }
        }
    }

}  // namespace our
