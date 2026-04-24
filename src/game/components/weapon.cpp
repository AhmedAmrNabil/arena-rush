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
        trailEnabled = data.value("trailEnabled", trailEnabled);
        trailMaxSegments = data.value("trailMaxSegments", trailMaxSegments);
        trailSegmentLifetime = data.value("trailSegmentLifetime", trailSegmentLifetime);
        trailHeadScale = data.value("trailHeadScale", trailHeadScale);
        trailTailScale = data.value("trailTailScale", trailTailScale);
        muzzleOffset = data.value("muzzleOffset", muzzleOffset);
        fireSound = data.value("fireSound", fireSound);
        reloadSound = data.value("reloadSound", reloadSound);
        reloadTime = data.value("reloadTime", reloadTime);
    }

}  // namespace gameplay
