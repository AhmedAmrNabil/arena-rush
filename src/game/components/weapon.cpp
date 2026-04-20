#include "weapon.hpp"

namespace gameplay {

    void WeaponComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) {
            return;
        }
        damage = data.value("damage", 0);
        range = data.value("range", 0.0f);
        fireRate = data.value("fireRate", 0.0f);
        maxAmmo = data.value("maxAmmo", 0);
        reloadTime = data.value("reloadTime", 0.0f);
        currentAmmo = maxAmmo;  // Initialize current ammo to max ammo when deserialized
    }

}  // namespace gameplay
