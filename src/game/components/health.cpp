#include "health.hpp"

namespace gameplay {

    void HealthComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        maxHealth = data.value("maxHealth", maxHealth);
        currentHealth = data.contains("currentHealth") ? data["currentHealth"].get<float>() : maxHealth;
        invulnerabilityTimer = data.value("invulnerabilityTimer", invulnerabilityTimer);
        isDead = data.value("isDead", isDead);
    }

}  // namespace gameplay
