#include "player-movement-system.hpp"

#include "collision-system.hpp"

namespace gameplay {

    void PlayerMovementSystem::handleSlidingAndCrouching(PlayerMovementComponent* movement, our::Keyboard& keyboard,
                                                         const glm::vec3& moveDir, float deltaTime,
                                                         float slideStartSpeed) {
        bool crouchPressed = keyboard.justPressed(GLFW_KEY_LEFT_CONTROL);

        if (movement->isSliding) {
            movement->slideTimer -= deltaTime;
            if (movement->slideTimer <= 0) {
                movement->isSliding = false;
                movement->isCrouching = true;
                movement->slideCooldownTimer = movement->slideCooldown;
            } else {
                // quadratically decays from slideStartSpeed to crouchSpeed
                float t =
                    movement->slideTimer /
                    movement->slideDuration;  // goes from 1 to 0 and used for the decay curve in speed when sliding
                float speed = movement->crouchSpeed + (slideStartSpeed - movement->crouchSpeed) * (t * t);
                movement->velocity = glm::normalize(movement->velocity) * speed;
            }
        } else if (crouchPressed) {
            if (movement->isSprinting && glm::length(moveDir) > 0.001f && movement->slideCooldownTimer <= 0) {
                movement->isSliding = true;
                movement->slideTimer = movement->slideDuration;
                movement->velocity =
                    moveDir * slideStartSpeed;  // preserves momentum when sliding and keys are released
                movement->isCrouching = false;
            } else {
                movement->isCrouching = !movement->isCrouching;
            }
        }
    }

    void PlayerMovementSystem::handleGroundedMovement(PlayerMovementComponent* movement, const glm::vec3& moveDir,
                                                      float deltaTime) {
        if (!movement->isSliding && movement->isGrounded) {
            float currentSpeed = movement->walkSpeed;
            if (movement->isSprinting) {
                movement->isCrouching = false;  // sprinting cancels crouching
                currentSpeed *= movement->sprintMultiplier;
            } else if (movement->isCrouching) {
                currentSpeed = movement->crouchSpeed;
            }

            if (glm::length(moveDir) > 0.001f) {
                movement->velocity = moveDir * currentSpeed;
            } else {
                // acts like friction so stopping after key release is not so stiff
                movement->velocity *= glm::max(0.0f, 1.0f - 10.0f * deltaTime);
                if (glm::length(movement->velocity) < 0.1f) movement->velocity = glm::vec3(0);
            }
        }
    }

    void PlayerMovementSystem::handleDashing(PlayerMovementComponent* movement, glm::vec3& playerPosition,
                                             our::Keyboard& keyboard, const glm::vec3& frontXZ) {
        if (keyboard.justPressed(GLFW_KEY_Q) && movement->dashCooldownTimer <= 0) {
            glm::vec3 dashDirection = glm::normalize(frontXZ);
            if (glm::length(movement->velocity) > 0.001f) {
                // dash along moving direction if not stationary
                dashDirection = glm::normalize(movement->velocity);
            }

            // Raycast to prevent tunneling through walls
            float actualDashDistance = movement->dashDistance;
            if (collisionSystem) {
                Ray dashRay{playerPosition - glm::vec3(0, movement->playerHeight / 2.0f, 0), dashDirection};
                HitInfo hit = collisionSystem->raycast(dashRay, movement->dashDistance, CollisionLayer::LAYER_ENVIRONMENT);
                
                if (hit.hit) {
                    actualDashDistance = glm::max(0.0f, hit.distance - 0.5f);
                }
            }

            playerPosition += dashDirection * actualDashDistance;
            movement->dashCooldownTimer = movement->dashCooldown;
            movement->dashTriggeredThisFrame = true;
        }
    }

    void PlayerMovementSystem::handleJumpingAndGravity(PlayerMovementComponent* movement, glm::vec3& playerPosition,
                                                       our::Keyboard& keyboard, float deltaTime) {
        if (keyboard.justPressed(GLFW_KEY_SPACE) && movement->isGrounded) {
            if (movement->isSliding) {
                movement->slideCooldownTimer = movement->slideCooldown;
            }
            movement->isSliding = false;  // jumping cancels crouching / sliding
            movement->isCrouching = false;
            movement->verticalVelocity = movement->jumpForce;
            movement->isGrounded = false;
        }

        if (!movement->isGrounded) {
            movement->verticalVelocity -= movement->gravity * deltaTime;
            playerPosition.y += movement->verticalVelocity * deltaTime;

            // jumping always goes back to standing height
            float minY = movement->groundLevel + movement->playerHeight;
            if (movement->verticalVelocity >= 0) {
                minY = movement->groundLevel + movement->crouchHeight;
            }

            if (playerPosition.y <= minY) {
                playerPosition.y = minY;
                movement->verticalVelocity = 0.0f;
                movement->isGrounded = true;
            }
        }
    }

    void PlayerMovementSystem::handleHeightInterpolation(PlayerMovementComponent* movement, glm::vec3& playerPosition,
                                                         float deltaTime) {
        if (movement->isGrounded) {
            float targetHeight =
                (movement->isCrouching || movement->isSliding) ? movement->crouchHeight : movement->playerHeight;
            float currentHeight = playerPosition.y - movement->groundLevel;
            float lerpedHeight = currentHeight + (targetHeight - currentHeight) * glm::min(1.0f, 10.0f * deltaTime);
            playerPosition.y = movement->groundLevel + lerpedHeight;
        }
    }

    float PlayerMovementSystem::getGroundHeight(const glm::vec3& position) {
        // clang-format off
        if (!collisionSystem) return 0.0f;

        // raycast downwards to find ground height
        Ray ray;
        ray.origin = position + glm::vec3(0, 0.5f, 0);  // start the ray a bit above the player's feet to avoid immediately hitting the ground
        ray.direction = glm::vec3(0, -1, 0);

        float maxDistance = 50.0f;  // maximum distance to check for ground below
        HitInfo hit = collisionSystem->raycast(ray, maxDistance, CollisionLayer::LAYER_ENVIRONMENT);

        if (hit.hit) return hit.point.y; // return the y-coordinate of the ground
        return 0.0f;  // default to 0 ground
        // clang-format on
    }

    void PlayerMovementSystem::update(our::World* world, float deltaTime, our::Application* app,
                                      CollisionSystem* collisionSystem) {
        this->collisionSystem = collisionSystem;

        PlayerMovementComponent* movement = nullptr;
        for (our::Entity* entity : world->getEntities()) {
            movement = entity->getComponent<PlayerMovementComponent>();
            if (movement) break;
        }

        if (!movement) return;

        our::Entity* playerEntity = movement->getOwner();
        glm::vec3& playerPosition = playerEntity->localTransform.position;

        float groundHeight = getGroundHeight(playerPosition);
        movement->groundLevel = groundHeight;

        our::Keyboard& keyboard = app->getKeyboard();
        movement->dashTriggeredThisFrame = false;

        if (movement->slideCooldownTimer > 0) movement->slideCooldownTimer -= deltaTime;
        if (movement->dashCooldownTimer > 0) movement->dashCooldownTimer -= deltaTime;

        glm::mat4 M = playerEntity->localTransform.toMat4();
        glm::vec3 front = glm::vec3(M * glm::vec4(0, 0, -1, 0));
        glm::vec3 right = glm::vec3(M * glm::vec4(1, 0, 0, 0));
        glm::vec3 frontXZ = glm::normalize(glm::vec3(front.x, 0, front.z));
        glm::vec3 rightXZ = glm::normalize(glm::vec3(right.x, 0, right.z));

        glm::vec3 moveDir(0);
        if (keyboard.isPressed(GLFW_KEY_W)) moveDir += frontXZ;
        if (keyboard.isPressed(GLFW_KEY_S)) moveDir -= frontXZ;
        if (keyboard.isPressed(GLFW_KEY_D)) moveDir += rightXZ;
        if (keyboard.isPressed(GLFW_KEY_A)) moveDir -= rightXZ;
        if (glm::length(moveDir) > 0.001f) moveDir = glm::normalize(moveDir);

        movement->isSprinting = keyboard.isPressed(GLFW_KEY_LEFT_SHIFT) && !movement->isSliding;

        float sprintSpeed = movement->walkSpeed * movement->sprintMultiplier;
        float slideStartSpeed = sprintSpeed * movement->slideSpeedMultiplier;

        handleSlidingAndCrouching(movement, keyboard, moveDir, deltaTime, slideStartSpeed);
        handleGroundedMovement(movement, moveDir, deltaTime);

        playerPosition += movement->velocity * deltaTime;

        handleDashing(movement, playerPosition, keyboard, frontXZ);
        handleJumpingAndGravity(movement, playerPosition, keyboard, deltaTime);
        handleHeightInterpolation(movement, playerPosition, deltaTime);
    }

}  // namespace gameplay
