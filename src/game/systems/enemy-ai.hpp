#pragma once

#include <GLFW/glfw3.h>

#include <application.hpp>
#include <cmath>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include "../components/collider.hpp"
#include "../components/enemy.hpp"
#include "../components/health.hpp"
#include "../components/weapon.hpp"
#include "projectile-system.hpp"

namespace gameplay {

    class EnemyAISystem {
        static glm::vec3 droneDirection(const glm::vec3& enemyPos, const glm::vec3& playerPos, float preferredDist) {
            glm::vec3 toPlayer = playerPos - enemyPos;
            toPlayer.y = 0.0f;

            float distance = glm::length(toPlayer);
            if (distance <= 0.0001f) return glm::vec3(0.0f);

            glm::vec3 direction = toPlayer / distance;
            float deadZone = glm::max(0.25f, preferredDist * 0.1f);

            if (distance > preferredDist + deadZone) return direction;
            if (distance < preferredDist - deadZone) return -direction;

            return glm::vec3(0.0f);
        }

        static void faceThePlayer(our::Entity* enemyEntity, const glm::vec3& movementDirection, float maxTurnStep) {
            if (maxTurnStep <= 0.0f || glm::dot(movementDirection, movementDirection) <= 0.000001f) return;

            float wanted = std::atan2(movementDirection.x, movementDirection.z);
            float& current = enemyEntity->localTransform.rotation.y;
            float yawDelta = std::atan2(std::sin(wanted - current), std::cos(wanted - current));

            current += glm::clamp(yawDelta, -maxTurnStep, maxTurnStep);
        }

    public:
        void update(our::World* world, our::Entity* playerEntity, our::Application* app, float deltaTime) {
            if (!world || !playerEntity || !app || deltaTime <= 0.0f) return;

            glm::vec3 playerPos = glm::vec3(playerEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
            float t = static_cast<float>(glfwGetTime());

            for (our::Entity* enemyEntity : world->getEntities()) {
                EnemyComponent* enemy = enemyEntity->getComponent<EnemyComponent>();
                if (!enemy) continue;

                HealthComponent* health = enemyEntity->getComponent<HealthComponent>();
                if (health && health->isDead) continue;

                glm::vec3 enemyPos = glm::vec3(enemyEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
                glm::vec3 toPlayer = playerPos - enemyPos;
                float distanceToPlayer = glm::length(toPlayer);
                glm::vec3 toPlayerXZ = toPlayer;
                toPlayerXZ.y = 0.0f;
                float XZDistance = glm::length(toPlayerXZ);

                glm::vec3 movementDirection = glm::vec3(0.0f);
                float movementSpeed = 0.0f;
                bool inAttackRange = distanceToPlayer <= enemy->attackRange;
                bool inAggroRange = distanceToPlayer <= enemy->aggroRange;
                glm::vec3 toPlayerDirection = (XZDistance > 0.0001f) ? (toPlayerXZ / XZDistance) : glm::vec3(0.0f);

                if (enemy->type == EnemyType::Flyer) {
                    if (inAggroRange) {
                        movementDirection = droneDirection(enemyPos, playerPos, enemy->preferredDistance);
                        movementSpeed = enemy->moveSpeed;
                    }

                    // fire at player
                    if (inAttackRange) {
                        if (WeaponComponent* weapon = enemyEntity->getComponent<WeaponComponent>()) {
                            glm::vec3 muzzleWorld =
                                glm::vec3(enemyEntity->getLocalToWorldMatrix() * glm::vec4(weapon->muzzleOffset, 1.0f));

                            ProjectileSystem::fire(world, app, enemyEntity, playerPos - muzzleWorld,
                                             CollisionLayer::LAYER_ENEMY);
                        }
                    }

                    if (!enemy->hoverOriginSet) {
                        enemy->hoverOriginY = enemyEntity->localTransform.position.y;
                        enemy->hoverOriginSet = true;
                    }

                    enemyEntity->localTransform.position.y =
                        enemy->hoverOriginY + enemy->baseHeight +
                        glm::sin(t * enemy->hoverFrequency) * enemy->hoverAmplitude;
                } else {
                    if (inAggroRange) movementSpeed = inAttackRange ? 0.0f : enemy->moveSpeed;

                    if (inAggroRange && !inAttackRange) movementDirection = toPlayerDirection;
                }

                faceThePlayer(enemyEntity, toPlayerDirection, enemy->turnSpeed * deltaTime);

                if (glm::dot(movementDirection, movementDirection) > 0.000001f) {
                    movementDirection = glm::normalize(movementDirection);
                    enemyEntity->localTransform.position += movementDirection * (movementSpeed * deltaTime);
                }
            }
        }
    };

}  // namespace gameplay
