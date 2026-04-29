#include "player.hpp"

namespace gameplay {

    void PlayerComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        currentActiveWeaponIndex = data.value("currentActiveWeaponIndex", currentActiveWeaponIndex);
    }

}  // namespace gameplay
