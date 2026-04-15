#include "player-movement.hpp"

namespace gameplay {

    void PlayerMovementComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        walkSpeed = data.value("walkSpeed", walkSpeed);
        groundLevel = data.value("groundLevel", groundLevel);
        playerHeight = data.value("playerHeight", playerHeight);

        sprintMultiplier = data.value("sprintMultiplier", sprintMultiplier);

        slideSpeed = data.value("slideSpeed", slideSpeed);
        slideDuration = data.value("slideDuration", slideDuration);
        slideCooldown = data.value("slideCooldown", slideCooldown);

        dashDistance = data.value("dashDistance", dashDistance);
        dashCooldown = data.value("dashCooldown", dashCooldown);

        jumpForce = data.value("jumpForce", jumpForce);
        gravity = data.value("gravity", gravity);
    }

}  // namespace gameplay
