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

#include "../components/enemy.hpp"
#include "../components/health.hpp"
#include "../components/weapon.hpp"
#include "collision-system.hpp"
#include "components/animation.hpp"
#include "components/audio-source.hpp"
#include "projectile-system.hpp"

namespace gameplay {

    class EnemyAISystem {
        // Steering Constants
        static constexpr int WHISKER_COUNT = 5;                 // rays in the fan
        static constexpr float WHISKER_HALF_ANGLE = 1.0472f;    // 60° — total fan = 120°
        static constexpr float WHISKER_LENGTH = 4.0f;           // base ray length
        static constexpr float WHISKER_HEIGHT_HIGH = 1.2f;      // chest-level ray
        static constexpr float WHISKER_HEIGHT_LOW = 0.4f;       // knee-level ray
        static constexpr float DANGER_WEIGHT = 1.5f;            // danger-vs-interest multiplier
        static constexpr float STRAFE_OBSTACLE_RAY_LEN = 5.0f;  // flyer obstacle check

        // Whisker Danger
        // Cast one horizontal ray from (enemyPos + height offset) in `dir`.
        // Returns a danger value in [0, DANGER_WEIGHT] based on proximity.
        // Closer hits produce higher danger.
        static float whiskerDanger(const glm::vec3& enemyPos, float height, const glm::vec3& dir, float rayLen,
                                   CollisionSystem* collision) {
            Ray ray;
            ray.origin = enemyPos + glm::vec3(0.0f, height, 0.0f);
            ray.direction = dir;

            HitInfo hit = collision->raycast(ray, rayLen, CollisionLayer::LAYER_ENVIRONMENT);
            if (hit.hit && hit.normal.y < 0.7f) {
                // Linear falloff: danger is maximum when hit is at origin,
                // zero when hit is at the ray's maximum length.
                return (1.0f - hit.distance / rayLen) * DANGER_WEIGHT;
            }
            return 0.0f;
        }

        // Context Steering (ground enemies)
        // Casts a fan of WHISKER_COUNT rays centered on `toPlayerDir`.
        // Each slot gets an interest score (alignment with player direction)
        // and a danger score (proximity to obstacles at two heights).
        // Returns the direction of the slot with the highest (interest − danger).
        // Sets `hasDanger` to true if any whisker detected an obstacle.
        static glm::vec3 contextSteer(const glm::vec3& enemyPos, const glm::vec3& toPlayerDir, float moveSpeed,
                                      CollisionSystem* collision, bool& hasDanger) {
            hasDanger = false;
            if (!collision) return toPlayerDir;

            float interest[WHISKER_COUNT];
            float danger[WHISKER_COUNT];
            glm::vec3 dirs[WHISKER_COUNT];

            // Forward = player direction (center slot will always have interest ≈ 1.0)
            glm::vec3 fwd =
                (glm::dot(toPlayerDir, toPlayerDir) > 0.0001f) ? glm::normalize(toPlayerDir) : glm::vec3(0, 0, 1);

            // Extend ray length slightly with move speed so faster enemies look further ahead
            float rayLen = WHISKER_LENGTH + moveSpeed * 0.3f;

            for (int i = 0; i < WHISKER_COUNT; ++i) {
                // Spread rays evenly across [−WHISKER_HALF_ANGLE, +WHISKER_HALF_ANGLE]
                float t = static_cast<float>(i) / static_cast<float>(WHISKER_COUNT - 1);
                float angle = -WHISKER_HALF_ANGLE + t * (2.0f * WHISKER_HALF_ANGLE);

                float cs = std::cos(angle);
                float sn = std::sin(angle);
                glm::vec3 dir(fwd.x * cs + fwd.z * sn, 0.0f, -fwd.x * sn + fwd.z * cs);
                dirs[i] = glm::normalize(dir);

                // Interest: cosine similarity with the player direction
                interest[i] = glm::max(0.0f, glm::dot(dirs[i], fwd));

                // Danger: worst of chest-level and knee-level casts
                float dHigh = whiskerDanger(enemyPos, WHISKER_HEIGHT_HIGH, dirs[i], rayLen, collision);
                float dLow = whiskerDanger(enemyPos, WHISKER_HEIGHT_LOW, dirs[i], rayLen, collision);
                danger[i] = glm::max(dHigh, dLow);

                if (danger[i] > 0.0f) hasDanger = true;
            }

            // Pick the slot with the best net score
            int bestSlot = WHISKER_COUNT / 2;
            float bestScore = -999.0f;
            for (int i = 0; i < WHISKER_COUNT; ++i) {
                float score = interest[i] - danger[i];
                if (score > bestScore) {
                    bestScore = score;
                    bestSlot = i;
                }
            }

            return dirs[bestSlot];
        }

        // Orbit-Strafe (flyers)
        // Produces a movement direction that orbits the player at `preferredDistance`
        // while periodically flipping strafe direction. Reverses immediately if
        // the forward path is blocked by an obstacle.
        static glm::vec3 orbitStrafe(const glm::vec3& enemyPos, const glm::vec3& playerPos, EnemyComponent* enemy,
                                     float deltaTime, CollisionSystem* collision) {
            glm::vec3 toPlayer = glm::vec3(playerPos.x - enemyPos.x, 0.0f, playerPos.z - enemyPos.z);
            float dist = glm::length(toPlayer);
            glm::vec3 toPlayerDir = (dist > 0.001f) ? (toPlayer / dist) : glm::vec3(0.0f, 0.0f, 1.0f);

            // Tangent = perpendicular on XZ, gives circular orbit
            glm::vec3 tangent = glm::cross(glm::vec3(0, 1, 0), toPlayerDir);
            if (glm::dot(tangent, tangent) <= 0.0001f) tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            tangent = glm::normalize(tangent) * static_cast<float>(enemy->strafeDirection);

            // Radial correction pushes toward/away from player to hold preferred distance
            float radialError = dist - enemy->preferredDistance;
            glm::vec3 radialCorrection = toPlayerDir * radialError * 0.5f;

            glm::vec3 moveDir = glm::normalize(tangent + radialCorrection);

            // Periodic strafe flip
            enemy->strafeFlipTimer -= deltaTime;
            if (enemy->strafeFlipTimer <= 0.0f) {
                enemy->strafeDirection *= -1;
                enemy->strafeFlipTimer = enemy->strafeFlipInterval;
            }

            // Single forward obstacle check — reverse strafe if blocked
            if (collision) {
                Ray ray;
                ray.origin = enemyPos + glm::vec3(0.0f, 1.0f, 0.0f);
                ray.direction = moveDir;
                HitInfo hit = collision->raycast(ray, STRAFE_OBSTACLE_RAY_LEN, CollisionLayer::LAYER_ENVIRONMENT);
                if (hit.hit && hit.distance < 2.0f) {
                    enemy->strafeDirection *= -1;
                    enemy->strafeFlipTimer = enemy->strafeFlipInterval;
                    tangent = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toPlayerDir)) *
                              static_cast<float>(enemy->strafeDirection);
                    moveDir = glm::normalize(tangent + radialCorrection);
                }
            }

            return moveDir;
        }

        // Facing
        // Smoothly rotates entity yaw toward `dir` by at most `maxTurnStep` radians.
        static void faceDirection(our::Entity* entity, const glm::vec3& dir, float maxTurnStep) {
            if (maxTurnStep <= 0.0f || glm::dot(dir, dir) <= 0.000001f) return;

            float wanted = std::atan2(dir.x, dir.z);
            float& current = entity->localTransform.rotation.y;
            float delta = std::atan2(std::sin(wanted - current), std::cos(wanted - current));

            current += glm::clamp(delta, -maxTurnStep, maxTurnStep);
        }

        // FSM Transition
        static void updateState(EnemyComponent* enemy, float distToPlayer) {
            using S = EnemyComponent::AIState;

            if (distToPlayer > enemy->aggroRange) {
                enemy->aiState = S::Idle;
            } else if (distToPlayer <= enemy->attackRange) {
                enemy->aiState = S::Attacking;
            } else {
                enemy->aiState = S::Aggro;
            }
        }

    public:
        void update(our::World* world, our::Entity* playerEntity, our::Application* app, float deltaTime,
                    CollisionSystem* collisionSystem = nullptr) {
            if (!world || !playerEntity || !app || deltaTime <= 0.0f) return;

            glm::vec3 playerPos = glm::vec3(playerEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
            HealthComponent* playerHealth = playerEntity->getComponent<HealthComponent>();
            if (playerHealth && playerHealth->isDead) return;

            for (our::Entity* enemyEntity : world->getEntities()) {
                EnemyComponent* enemy = enemyEntity->getComponent<EnemyComponent>();
                if (!enemy) continue;

                enemy->attackTimer = glm::max(0.0f, enemy->attackTimer - deltaTime);

                HealthComponent* health = enemyEntity->getComponent<HealthComponent>();
                our::AnimationComponent* anim = enemyEntity->getComponent<our::AnimationComponent>();

                if (health && health->isDead) continue;

                glm::vec3 enemyPos = glm::vec3(enemyEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
                glm::vec3 toPlayer = playerPos - enemyPos;
                float distanceToPlayer = glm::length(toPlayer);

                // Flatten to XZ for horizontal steering
                glm::vec3 toPlayerXZ = glm::vec3(toPlayer.x, 0.0f, toPlayer.z);
                float XZDistance = glm::length(toPlayerXZ);
                glm::vec3 toPlayerDir = (XZDistance > 0.0001f) ? (toPlayerXZ / XZDistance) : glm::vec3(0.0f);

                // FSM
                updateState(enemy, distanceToPlayer);

                glm::vec3 movementDirection(0.0f);
                float movementSpeed = 0.0f;
                glm::vec3 faceDir = toPlayerDir;

                using S = EnemyComponent::AIState;

                if (enemy->type == EnemyType::Flyer) {
                    // FLYER
                    switch (enemy->aiState) {
                        case S::Idle:
                            if (anim) anim->setNextState(our::AnimationState::Idle);
                            break;

                        case S::Aggro:
                            movementDirection = orbitStrafe(enemyPos, playerPos, enemy, deltaTime, collisionSystem);
                            movementSpeed = enemy->moveSpeed;
                            if (anim) anim->setNextCommand({our::AnimationState::Walk, -1.0f, movementSpeed});
                            break;

                        case S::Attacking:
                            movementDirection = orbitStrafe(enemyPos, playerPos, enemy, deltaTime, collisionSystem);
                            movementSpeed = enemy->moveSpeed * 0.5f;

                            if (WeaponComponent* weapon = enemyEntity->getComponent<WeaponComponent>()) {
                                if (enemy->attackTimer <= 0.0f) {
                                    if (anim) anim->playAttack(enemy->attackCooldown);
                                    glm::vec3 muzzleWorld = glm::vec3(enemyEntity->getLocalToWorldMatrix() *
                                                                      glm::vec4(weapon->muzzleOffset, 1.0f));
                                    ProjectileSystem::fire(world, app, enemyEntity, playerPos - muzzleWorld,
                                                           CollisionLayer::LAYER_ENEMY);
                                    enemy->attackTimer = enemy->attackCooldown;
                                } else {
                                    if (anim) anim->setNextState(our::AnimationState::Idle);
                                }
                            }
                            break;
                    }

                    // Flyer always faces the player
                    faceDir = toPlayerDir;

                    // Hover bob
                    if (!enemy->hoverOriginSet) {
                        enemy->hoverOriginY = enemyEntity->localTransform.position.y;
                        enemy->hoverOriginSet = true;
                    }
                    enemyEntity->localTransform.position.y = enemy->hoverOriginY + enemy->baseHeight;

                } else {
                    // GROUND (Brute / Charger)
                    switch (enemy->aiState) {
                        case S::Idle:
                            if (anim) anim->setNextState(our::AnimationState::Idle);
                            break;

                        case S::Aggro: {
                            bool dangerDetected = false;
                            glm::vec3 steerDir =
                                contextSteer(enemyPos, toPlayerDir, enemy->moveSpeed, collisionSystem, dangerDetected);

                            movementDirection = steerDir;
                            movementSpeed = enemy->moveSpeed;

                            // Face avoidance direction when dodging, player when clear
                            faceDir = dangerDetected ? steerDir : toPlayerDir;
                            if (anim) anim->setNextCommand({our::AnimationState::Walk, -1.0f, movementSpeed});
                            break;
                        }

                        case S::Attacking: {
                            faceDir = toPlayerDir;
                            // Integrate the melee damage from main directly into the FSM Attacking state
                            if (enemy->attackTimer <= 0.0f) {
                                playerHealth->takeDamage(enemy->attackDamage);
                                if (anim) anim->playAttack(enemy->attackCooldown);

                                // Play sound
                                our::AudioBuffer* buffer = our::AssetLoader<our::AudioBuffer>::get(enemy->attackSound);
                                if (buffer) {
                                    app->getAudioSystem().playSound(buffer, enemyPos, 0.5f, 1.0f, false, 6.0f, 60.0f);
                                }

                                enemy->attackTimer = enemy->attackCooldown;
                            } else {
                                if (anim) anim->setNextState(our::AnimationState::Idle);
                            }
                            break;
                        }
                    }

                    // Ground tracking
                    if (collisionSystem) {
                        float lookAheadDist = 0.5f;  // to overcome simple obstacles like the pavement

                        Ray downRay;
                        downRay.origin = enemyPos + glm::vec3(0.0f, 1.0f, 0.0f) + (faceDir * lookAheadDist);
                        downRay.direction = glm::vec3(0.0f, -1.0f, 0.0f);
                        HitInfo groundHit = collisionSystem->raycast(downRay, 50.0f, CollisionLayer::LAYER_ENVIRONMENT);
                        if (groundHit.hit) {
                            float targetY = groundHit.point.y;
                            float currentY = enemyEntity->localTransform.position.y;
                            enemyEntity->localTransform.position.y =
                                glm::mix(currentY, targetY, glm::clamp(15.0f * deltaTime, 0.0f, 1.0f));
                        }
                    }
                }

                // Apply facing
                faceDirection(enemyEntity, faceDir, enemy->turnSpeed * deltaTime);

                // Apply movement (XZ only)
                if (movementSpeed > 0.0f && glm::dot(movementDirection, movementDirection) > 0.000001f) {
                    glm::vec3 move = glm::normalize(movementDirection) * (movementSpeed * deltaTime);
                    enemyEntity->localTransform.position.x += move.x;
                    enemyEntity->localTransform.position.z += move.z;
                }
            }
        }
    };

}  // namespace gameplay
