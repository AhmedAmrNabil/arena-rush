#include "model-renderer.hpp"

#include "../asset-loader.hpp"

namespace our {
    // Receives the model & material from the AssetLoader by the names given in the json object
    void ModelRendererComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        // Notice how we just get a string from the json file and pass it to the AssetLoader to get us the actual asset
        model = AssetLoader<Model>::get(data["model"].get<std::string>());
        material = AssetLoader<Material>::get(data["material"].get<std::string>());
    }
}  // namespace our
