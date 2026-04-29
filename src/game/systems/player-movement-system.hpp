#pragma once

#include <application.hpp>
#include <components/player-movement.hpp>
#include <ecs/world.hpp>
#include <glm/glm.hpp>
#include <input/joystick.hpp>
#include <input/keyboard.hpp>

namespace gameplay {
    class CollisionSystem;  // forward declaration

    // Handles all player movement (walking, sprinting, sliding, dashing)
    // operates on the entity with player movement and camera components
    class PlayerMovementSystem {
        CollisionSystem* collisionSystem = nullptr;  // for raycasting checks

        void handleSlidingAndCrouching(PlayerMovementComponent* movement, our::Keyboard& keyboard,
                                       our::Joystick& joystick, const glm::vec3& moveDir, float deltaTime,
                                       float slideStartSpeed);
        void handleGroundedMovement(PlayerMovementComponent* movement, const glm::vec3& moveDir, float deltaTime);
        void handleDashing(PlayerMovementComponent* movement, glm::vec3& playerPosition, our::Keyboard& keyboard,
                           const glm::vec3& frontXZ, const glm::vec3& moveDir, our::Joystick& joystick);
        void handleJumpingAndGravity(PlayerMovementComponent* movement, glm::vec3& playerPosition,
                                     our::Keyboard& keyboard, our::Joystick& joystick, float deltaTime);
        void handleHeightInterpolation(PlayerMovementComponent* movement, glm::vec3& playerPosition, float deltaTime);

        float getGroundHeight(const glm::vec3& position);

    public:
        void update(our::World* world, float deltaTime, our::Application* app, CollisionSystem* collisionSystem);
    };

}  // namespace gameplay
