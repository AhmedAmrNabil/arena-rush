#pragma once

#include <glm/glm.hpp>

#include <ecs/component.hpp>

namespace gameplay {

    class PlayerMovementComponent : public our::Component {
    public:
        float walkSpeed = 5.0f;
        float groundLevel = 0.0f;
        float playerHeight = 1.7f;

        float sprintMultiplier = 2.0f;

        float slideSpeed = 10.0f;
        float slideDuration = 0.6f;
        float slideCooldown = 0.3f;

        float dashDistance = 5.0f;
        float dashCooldown = 1.5f;

        glm::vec3 velocity = {0, 0, 0};
        bool isSprinting = false;
        bool isSliding = false;
        float slideTimer = 0.0f;
        float slideCooldownTimer = 0.0f;
        float dashCooldownTimer = 0.0f;

        static std::string getID() {
            return "PlayerMovement";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
