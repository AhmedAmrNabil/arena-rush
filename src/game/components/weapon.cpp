#include "weapon.hpp"

#include <deserialize-utils.hpp>

namespace gameplay {

    void WeaponComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        cooldown = data.value("cooldown", cooldown);
        bulletSpeed = data.value("bulletSpeed", bulletSpeed);
        bulletDamage = data.value("bulletDamage", bulletDamage);
        bulletLifetime = data.value("bulletLifetime", bulletLifetime);
        bulletScale = data.value("bulletScale", bulletScale);
        bulletColliderRadius = data.value("bulletColliderRadius", bulletColliderRadius);
        muzzleOffset = data.value("muzzleOffset", muzzleOffset);
        fireSound = data.value("fireSound", fireSound);
    }

}  // namespace gameplay
