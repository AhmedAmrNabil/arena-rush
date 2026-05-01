#pragma once

#include <ecs/component.hpp>
#include <glm/glm.hpp>

namespace gameplay {

    enum class EnemyType { Brute, Charger, Flyer };

    class EnemyComponent : public our::Component {
    public:
        EnemyType type = EnemyType::Brute;
        float moveSpeed = 2.0f;
        float acceleration = 8.0f;
        float turnSpeed = 6.0f;
        float aggroRange = 15.0f;
        float attackRange = 1.5f;
        float attackCooldown = 2.0f;
        float attackTimer = 0.0f;
        float attackDamage = 10.0f;
        float jumpForce = 11.0f;
        float gravity = 20.0f;
        float jumpCooldown = 1.0f;
        float preferredDistance = 4.0f;
        float hoverFrequency = 2.0f;
        float hoverAmplitude = 0.5f;
        float baseHeight = 3.0f;
        bool hoverOriginSet = false;
        float hoverOriginY = 0.0f;
        int scoreValue = 100;
        std::string attackSound = "monster-attack";

        // Runtime state (not serialized)
        enum class AIState { Idle, Aggro, Attacking };
        AIState aiState = AIState::Idle;
        float verticalVelocity = 0.0f;
        bool isGrounded = true;
        float jumpCooldownTimer = 0.0f;
        float stuckTimer = 0.0f;
        bool lastPositionSet = false;
        glm::vec3 lastPosition = glm::vec3(0.0f);

        // Flyer orbit-strafe
        int strafeDirection = 1;          // +1 CW, −1 CCW around player
        float strafeFlipTimer = 0.0f;     // seconds until next random flip
        float strafeFlipInterval = 3.0f;  // how often to flip (seconds)

        static std::string getID() {
            return "Enemy";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
