#pragma once

#include <ecs/component.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace gameplay {

    class WeaponComponent : public our::Component {
    public:
        float cooldown = 0.25f;
        float timer = 0.0f;  // 0 means ready to fire
        float bulletSpeed = 45.0f;
        float bulletDamage = 15.0f;
        float bulletLifetime = 3.0f;
        float bulletScale = 0.15f;
        float bulletColliderRadius = 0.15f;

        glm::vec3 muzzleOffset = glm::vec3(0.0f, 0.0f, 0.0f);
        std::string fireSound;
        static std::string getID() {
            return "Weapon";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
