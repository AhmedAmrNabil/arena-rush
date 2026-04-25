#include "player.hpp"

namespace gameplay {

    void PlayerComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        magSize = data.value("magSize", magSize);
        maxAmmo = data.value("maxAmmo", maxAmmo);
        currentAmmo = magSize;
    }

}  // namespace gameplay
