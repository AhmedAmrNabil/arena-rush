#pragma once

#include <application.hpp>
#include <components/camera.hpp>
#include <ecs/world.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include "GLFW/glfw3.h"
#include "components/player-movement.hpp"
#include "components/post-process-effects.hpp"
#include "glm/fwd.hpp"
#include "input/keyboard.hpp"

namespace gameplay {

    // Handles all player movement (walking, sprinting, sliding, dashing)
    // operates on the entity with player movement and camera components
    class PlayerMovementSystem {
        float dashEffectTimer = 0.0f;
        static constexpr float DASH_EFFECT_DURATION = 0.3f;

    public:
        void update(our::World* world, float deltaTime, our::Application* app) {
            PlayerMovementComponent* movement = nullptr;
            our::CameraComponent* camera = nullptr;
            for (auto entity : world->getEntities()) {
                movement = entity->getComponent<PlayerMovementComponent>();
                camera = entity->getComponent<our::CameraComponent>();
                if (movement && camera) break;
            }

            if (!(movement && camera)) return;

            our::Entity* playerEntity = camera->getOwner();
            glm::vec3& playerPosition = playerEntity->localTransform.position;

            our::Keyboard& keyboard = app->getKeyboard();

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
            bool crouchPressed = keyboard.justPressed(GLFW_KEY_LEFT_CONTROL);
            float sprintSpeed = movement->walkSpeed * movement->sprintMultiplier;
            float slideStartSpeed = sprintSpeed * movement->slideSpeedMultiplier;

            if (movement->isSliding) {
                movement->slideTimer -= deltaTime;
                if (movement->slideTimer <= 0) {
                    movement->isSliding = false;
                    movement->isCrouching = true;
                    movement->slideCooldownTimer = movement->slideCooldown;
                } else {
                    // quadratically decays from slideStartSpeed to crouchSpeed
                    float t = movement->slideTimer / movement->slideDuration;
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
            if (!movement->isSliding && movement->isGrounded) {
                float currentSpeed = movement->walkSpeed;
                if (movement->isCrouching) {
                    currentSpeed = movement->crouchSpeed;
                } else if (movement->isSprinting) {
                    currentSpeed *= movement->sprintMultiplier;
                }

                if (glm::length(moveDir) > 0.001f) {
                    movement->velocity = moveDir * currentSpeed;
                } else {
                    // acts like friction so stopping after key release is not so stiff
                    movement->velocity *= glm::max(0.0f, 1.0f - 10.0f * deltaTime);
                    if (glm::length(movement->velocity) < 0.1f) movement->velocity = glm::vec3(0);
                }
            }

            playerPosition += movement->velocity * deltaTime;

            if (keyboard.justPressed(GLFW_KEY_Q) && movement->dashCooldownTimer <= 0) {
                glm::vec3 dashDirection = glm::normalize(frontXZ);
                if (glm::length(movement->velocity) > 0.001f) {
                    // dash along moving direction if not stationary
                    dashDirection = glm::normalize(movement->velocity);
                }
                playerPosition += dashDirection * movement->dashDistance;
                movement->dashCooldownTimer = movement->dashCooldown;
                dashEffectTimer = DASH_EFFECT_DURATION;
            }

            if (dashEffectTimer > 0) dashEffectTimer -= deltaTime;
            if (auto* effects = playerEntity->getComponent<PostProcessEffectsComponent>()) {
                effects->uniforms["intensity"] = glm::max(0.0f, dashEffectTimer / DASH_EFFECT_DURATION);
            }
            if (keyboard.justPressed(GLFW_KEY_SPACE) && movement->isGrounded) {
                if (movement->isSliding) {
                    movement->slideCooldownTimer = movement->slideCooldown;
                }
                movement->isSliding = false;
                movement->isCrouching = false;
                movement->verticalVelocity = movement->jumpForce;
                movement->isGrounded = false;
            }

            if (!movement->isGrounded) {
                movement->verticalVelocity -= movement->gravity * deltaTime;
                playerPosition.y += movement->verticalVelocity * deltaTime;

                // jumping always goes back to standing height
                float minY = movement->verticalVelocity < 0 ? movement->groundLevel + movement->playerHeight
                                                            : movement->groundLevel + movement->crouchHeight;
                if (playerPosition.y <= minY) {
                    playerPosition.y = minY;
                    movement->verticalVelocity = 0.0f;
                    movement->isGrounded = true;
                }
            }

            if (movement->isGrounded) {
                float targetHeight =
                    (movement->isCrouching || movement->isSliding) ? movement->crouchHeight : movement->playerHeight;
                float currentHeight = playerPosition.y - movement->groundLevel;
                float lerpedHeight = currentHeight + (targetHeight - currentHeight) * glm::min(1.0f, 10.0f * deltaTime);
                playerPosition.y = movement->groundLevel + lerpedHeight;
            }
        }
    };

}  // namespace gameplay
