#include "health.hpp"

#include <algorithm>

namespace gameplay {

    void HealthComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        maxHealth = data.value("maxHealth", maxHealth);
        currentHealth = data.contains("currentHealth") ? data["currentHealth"].get<float>() : maxHealth;
        damageRevealTimer = data.value("damageRevealTimer", damageRevealTimer);
        isDead = data.value("isDead", isDead);
    }

    float HealthComponent::takeDamage(float amount) {
        if (isDead || amount <= 0.0f) return 0.0f;

        float x = std::min(amount, currentHealth);
        currentHealth -= x;
        if (x > 0.0f) damageRevealTimer = 2.5f;

        if (currentHealth <= 0.0f) {
            currentHealth = 0.0f;
            isDead = true;
        }

        return x;
    }

    float HealthComponent::getHealthRatio() const {
        if (maxHealth <= 0.0f) return 0.0f;
        return std::clamp(currentHealth / maxHealth, 0.0f, 1.0f);
    }

}  // namespace gameplay
