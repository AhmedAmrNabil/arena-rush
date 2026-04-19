#pragma once

#include <application.hpp>
#include <components/player-movement.hpp>
#include <ecs/world.hpp>
#include <glm/glm.hpp>
#include <input/keyboard.hpp>

namespace gameplay {

    // Handles all player movement (walking, sprinting, sliding, dashing)
    // operates on the entity with player movement and camera components
    class PlayerMovementSystem {
        void handleSlidingAndCrouching(PlayerMovementComponent* movement, our::Keyboard& keyboard,
                                       const glm::vec3& moveDir, float deltaTime, float slideStartSpeed);
        void handleGroundedMovement(PlayerMovementComponent* movement, const glm::vec3& moveDir, float deltaTime);
        void handleDashing(PlayerMovementComponent* movement, glm::vec3& playerPosition, our::Keyboard& keyboard,
                           const glm::vec3& frontXZ);
        void handleJumpingAndGravity(PlayerMovementComponent* movement, glm::vec3& playerPosition,
                                     our::Keyboard& keyboard, float deltaTime);
        void handleHeightInterpolation(PlayerMovementComponent* movement, glm::vec3& playerPosition, float deltaTime);

    public:
        void update(our::World* world, float deltaTime, our::Application* app);
    };

}  // namespace gameplay
