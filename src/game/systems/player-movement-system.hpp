#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include <application.hpp>
#include <components/camera.hpp>
#include <ecs/world.hpp>

#include "GLFW/glfw3.h"
#include "components/player-movement.hpp"
#include "input/keyboard.hpp"

namespace gameplay {

    // Handles all player movement (walking, sprinting, sliding, dashing)
    // operates on the entity with player movement and camera components
    class PlayerMovementSystem {
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

            if (movement->isSliding) {
                movement->slideTimer -= deltaTime;
                if (movement->slideTimer <= 0) {
                    movement->isSliding = false;
                    movement->slideCooldownTimer = movement->slideCooldown;
                    movement->velocity = glm::vec3(0);
                } else {
                    float t = movement->slideTimer / movement->slideDuration;
                    playerPosition += movement->velocity * t * deltaTime;
                }
            } else {
                float currentSpeed = movement->walkSpeed;
                movement->isSprinting = keyboard.isPressed(GLFW_KEY_LEFT_SHIFT);
                if (movement->isSprinting) {
                    currentSpeed *= movement->sprintMultiplier;
                }

                glm::vec3 moveDir(0);
                if (keyboard.isPressed(GLFW_KEY_W)) moveDir += frontXZ;
                if (keyboard.isPressed(GLFW_KEY_S)) moveDir -= frontXZ;
                if (keyboard.isPressed(GLFW_KEY_D)) moveDir += rightXZ;
                if (keyboard.isPressed(GLFW_KEY_A)) moveDir -= rightXZ;

                if (glm::length(moveDir) > 0.001f) {
                    moveDir = glm::normalize(moveDir);
                    playerPosition += moveDir * currentSpeed * deltaTime;
                }

                if (keyboard.justPressed(GLFW_KEY_TAB) && glm::length(moveDir) > 0.001f &&
                    movement->slideCooldownTimer <= 0) {
                    movement->isSliding = true;
                    movement->slideTimer = movement->slideDuration;
                    movement->velocity = moveDir * movement->slideSpeed;
                }
            }

            // should be able to dash regardless of sliding / walking
            if (keyboard.justPressed(GLFW_KEY_Q) && movement->dashCooldownTimer <= 0) {
                playerPosition += frontXZ * movement->dashDistance; // make it a displacement vector then add to position
                movement->dashCooldownTimer = movement->dashCooldown;
            }

            // we can jump regardless of sliding/dashing/walking
            if (keyboard.justPressed(GLFW_KEY_SPACE) && movement->isGrounded) {
                movement->verticalVelocity = movement->jumpForce;
                movement->isGrounded = false;
            }

            movement->verticalVelocity -= movement->gravity * deltaTime;
            playerPosition.y += movement->verticalVelocity * deltaTime;

            float minY = movement->groundLevel + movement->playerHeight;
            if (playerPosition.y <= minY) {
                playerPosition.y = minY;
                movement->verticalVelocity = 0.0f;
                movement->isGrounded = true;
            }
        }
    };

}  // namespace gameplay
