#include "model-renderer.hpp"

namespace our {

    void ModelRendererComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        model = AssetLoader<Model>::get(data["model"].get<std::string>());
        firstPersonOverlay = data.value("firstPersonOverlay", firstPersonOverlay);
    }

}  // namespace our
