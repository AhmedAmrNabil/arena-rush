#include "player.hpp"

namespace gameplay {

    void PlayerComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        radius = data.value("radius", radius);
        height = data.value("height", height);
    }

}  // namespace gameplay
