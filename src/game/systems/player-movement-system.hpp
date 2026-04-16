#pragma once

#include <application.hpp>
#include <components/player-movement.hpp>
#include <components/post-process-effects.hpp>
#include <ecs/world.hpp>
#include <glm/glm.hpp>
#include <input/keyboard.hpp>

namespace gameplay {

    // Handles all player movement (walking, sprinting, sliding, dashing)
    // operates on the entity with player movement and camera components
    class PlayerMovementSystem {
        float dashEffectTimer = 0.0f;
        static constexpr float DASH_EFFECT_DURATION = 0.3f;

        void handleSlidingAndCrouching(PlayerMovementComponent* movement, our::Keyboard& keyboard,
                                       const glm::vec3& moveDir, float deltaTime, float slideStartSpeed);
        void handleGroundedMovement(PlayerMovementComponent* movement, const glm::vec3& moveDir, float deltaTime);
        void handleDashing(PlayerMovementComponent* movement, our::Entity* playerEntity, glm::vec3& playerPosition,
                           our::Keyboard& keyboard, const glm::vec3& frontXZ, float deltaTime);
        void handleJumpingAndGravity(PlayerMovementComponent* movement, glm::vec3& playerPosition,
                                     our::Keyboard& keyboard, float deltaTime);
        void handleHeightInterpolation(PlayerMovementComponent* movement, glm::vec3& playerPosition, float deltaTime);

    public:
        void update(our::World* world, float deltaTime, our::Application* app);
    };

}  // namespace gameplay
