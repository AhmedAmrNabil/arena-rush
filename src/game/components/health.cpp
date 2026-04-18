#include "health.hpp"

namespace gameplay {

    void HealthComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        maxHealth = data.value("maxHealth", maxHealth);
        currentHealth = data.value("currentHealth", currentHealth);
        invulnerabilityTimer = data.value("invulnerabilityTimer", invulnerabilityTimer);
        isDead = data.value("isDead", isDead);
    }

}  // namespace gameplay
