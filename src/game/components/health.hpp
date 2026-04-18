#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class HealthComponent : public our::Component {
    public:
        float maxHealth = 100.0f;
        float currentHealth = 100.0f;
        float invulnerabilityTimer = 0.0f;
        float damageRevealTimer = 0.0f;
        bool isDead = false;

        float getHealthRatio() const;

        static std::string getID() {
            return "Health";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
