#include "weapon.hpp"

namespace gameplay {

    void WeaponComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) {
            return;
        }
        damage = data.value("damage", 0);
        range = data.value("range", 0.0f);
        fireRate = data.value("fireRate", 0.0f);
    }

}  // namespace gameplay
