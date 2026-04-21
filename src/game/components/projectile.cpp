#include "projectile.hpp"

namespace gameplay {

    void ProjectileComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        damage = data.value("damage", damage);
        lifetime = data.value("lifetime", lifetime);
    }

}  // namespace gameplay
