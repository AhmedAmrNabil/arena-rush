#pragma once

#include "../asset-loader.hpp"
#include "../ecs/component.hpp"
#include "../material/material.hpp"
#include "../model/model.hpp"

namespace our {

    class ModelRendererComponent : public Component {
    public:
        Model* model;  // The model that should be drawn

        // The ID of this component type is "Model Renderer"
        static std::string getID() {
            return "Model Renderer";
        }

        // Receives the model from the AssetLoader by the name given in the json object
        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace our
