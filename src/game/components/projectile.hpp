#pragma once

#include <ecs/component.hpp>
#include <glm/vec3.hpp>

#include "collider.hpp"

namespace gameplay {

    class ProjectileComponent : public our::Component {
    public:
        glm::vec3 velocity = glm::vec3(0.0f);
        float damage = 10.0f;
        float lifetime = 3.0f;
        CollisionLayer shooterLayer = LAYER_ENVIRONMENT;

        static std::string getID() {
            return "Projectile";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
