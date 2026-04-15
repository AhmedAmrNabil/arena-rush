#pragma once

#include <ecs/component.hpp>
#include <glm/glm.hpp>

namespace gameplay {

    class PlayerMovementComponent : public our::Component {
    public:
        float walkSpeed = 5.0f;
        float groundLevel = 0.0f;
        float playerHeight = 1.7f;

        float sprintMultiplier = 2.0f;

        float crouchSpeed = 3.0f;
        float crouchHeight = 1.0f;

        float slideSpeedMultiplier = 1.5f;
        float slideDuration = 1.0f;
        float slideCooldown = 0.3f;

        float dashDistance = 5.0f;
        float dashCooldown = 1.5f;

        float jumpForce = 8.0f;
        float gravity = 20.0f;  // decrease for floatiness, can be a powerup easily

        glm::vec3 velocity = {0, 0, 0};
        bool isSprinting = false;
        bool isCrouching = false;
        bool isSliding = false;
        float slideTimer = 0.0f;
        float slideCooldownTimer = 0.0f;
        float dashCooldownTimer = 0.0f;
        float verticalVelocity = 0.0f;
        bool isGrounded = true;

        static std::string getID() {
            return "PlayerMovement";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
