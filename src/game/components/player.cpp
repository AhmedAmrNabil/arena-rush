#include "player.hpp"

namespace gameplay {

    void PlayerComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        currentAmmo = data.value("currentAmmo", currentAmmo);
        maxAmmo = data.value("maxAmmo", maxAmmo);
    }

}  // namespace gameplay
