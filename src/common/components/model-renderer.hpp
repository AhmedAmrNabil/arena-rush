#pragma once

#include "../asset-loader.hpp"
#include "../ecs/component.hpp"
#include "../material/material.hpp"
#include "../model/model.hpp"

namespace our {

    // This component denotes that any renderer should draw the given model using the given material at the
    // transformation of the owning entity.
    class ModelRendererComponent : public Component {
    public:
        Model* model;        // The model that should be drawn
        Material* material;  // The material used to draw the model

        // The ID of this component type is "Model Renderer"
        static std::string getID() {
            return "Model Renderer";
        }

        // Receives the model & material from the AssetLoader by the names given in the json object
        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace our
