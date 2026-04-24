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
#include "projectile-system.hpp"

namespace gameplay {

    class EnemyAISystem {
        // ─── Constants ───────────────────────────────────────────────────
        static constexpr int WHISKER_COUNT = 5;                 // number of rays in the fan
        static constexpr float WHISKER_HALF_ANGLE = 1.0472f;    // 60° in radians  (total fan = 120°)
        static constexpr float WHISKER_LENGTH = 4.0f;           // base ray length
        static constexpr float WHISKER_HEIGHT_HIGH = 1.2f;      // chest-level ray (detects tall walls)
        static constexpr float WHISKER_HEIGHT_LOW = 0.4f;       // knee-level ray  (detects short walls)
        static constexpr float DANGER_WEIGHT = 1.5f;            // how strongly danger overrides interest
        static constexpr float FLOOR_NORMAL_THRESHOLD = 0.5f;   // hit normal Y above this = floor, not wall
        static constexpr float STEP_OVER_HEIGHT = 0.25f;        // hits below this height above feet = curb, ignore
        static constexpr float STRAFE_OBSTACLE_RAY_LEN = 5.0f;  // flyer forward obstacle check

        // Helper: cast one whisker ray at a given height and return the danger value
        static float whiskerDanger(const glm::vec3& enemyPos, float height, const glm::vec3& dir, float rayLen,
                                   float stepOverY, CollisionSystem* collision) {
            Ray ray;
            ray.origin = enemyPos + glm::vec3(0.0f, height, 0.0f);
            ray.direction = dir;

            HitInfo hit = collision->raycast(ray, rayLen, CollisionLayer::LAYER_ENVIRONMENT);
            if (hit.hit) {
                bool isFloor = hit.normal.y >= FLOOR_NORMAL_THRESHOLD;
                bool isBelowStep = hit.point.y < stepOverY;
                if (!isFloor && !isBelowStep) {
                    return (1.0f - hit.distance / rayLen) * DANGER_WEIGHT;
                }
            }
            return 0.0f;
        }

        // ─── Context Steering (ground enemies) ──────────────────────────
        // Fan is centered on toPlayerDir so interest is always meaningful.
        // Casts TWO rays per slot (chest + knee) to detect both tall and
        // short walls. Sets hasDanger=true if any whisker detected a wall.
        static glm::vec3 contextSteer(const glm::vec3& enemyPos, const glm::vec3& toPlayerDir, float moveSpeed,
                                      CollisionSystem* collision, bool& hasDanger) {
            hasDanger = false;

            // Fallback: if no collision system just go straight toward player
            if (!collision) return toPlayerDir;

            float interest[WHISKER_COUNT];
            float danger[WHISKER_COUNT];
            glm::vec3 dirs[WHISKER_COUNT];

            // Fan is centered on the player direction — guarantees the
            // center slot always has maximum interest (1.0).
            glm::vec3 fwd =
                (glm::dot(toPlayerDir, toPlayerDir) > 0.0001f) ? glm::normalize(toPlayerDir) : glm::vec3(0, 0, 1);

            float stepOverY = enemyPos.y + STEP_OVER_HEIGHT;
            float rayLen = WHISKER_LENGTH + moveSpeed * 0.3f;

            for (int i = 0; i < WHISKER_COUNT; ++i) {
                float t = (WHISKER_COUNT == 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(WHISKER_COUNT - 1);
                float angle = -WHISKER_HALF_ANGLE + t * (2.0f * WHISKER_HALF_ANGLE);

                float cs = std::cos(angle);
                float sn = std::sin(angle);
                glm::vec3 dir(fwd.x * cs + fwd.z * sn, 0.0f, -fwd.x * sn + fwd.z * cs);
                if (glm::dot(dir, dir) > 0.0001f) dir = glm::normalize(dir);
                dirs[i] = dir;

                // Interest = how closely this slot points toward the player
                interest[i] = glm::max(0.0f, glm::dot(dir, fwd));

                // Danger: max of chest-level and knee-level raycasts
                float dangerHigh = whiskerDanger(enemyPos, WHISKER_HEIGHT_HIGH, dir, rayLen, stepOverY, collision);
                float dangerLow = whiskerDanger(enemyPos, WHISKER_HEIGHT_LOW, dir, rayLen, stepOverY, collision);
                danger[i] = glm::max(dangerHigh, dangerLow);
                if (danger[i] > 0.0f) hasDanger = true;
            }

            // Pick slot with highest (interest − danger)
            int bestSlot = WHISKER_COUNT / 2;  // default to center (toward player)
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

        // ─── Orbit-Strafe (flyers) ──────────────────────────────────────
        static glm::vec3 orbitStrafe(const glm::vec3& enemyPos, const glm::vec3& playerPos, EnemyComponent* enemy,
                                     float deltaTime, CollisionSystem* collision) {
            glm::vec3 toPlayer = playerPos - enemyPos;
            float dist = glm::length(toPlayer);
            if (dist < 0.001f) return glm::vec3(0.0f);

            glm::vec3 toPlayerDir = toPlayer / dist;

            // Tangent (perpendicular on XZ plane) for orbiting
            glm::vec3 tangent = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toPlayerDir)) *
                                static_cast<float>(enemy->strafeDirection);

            // Radial correction: push toward / away to maintain preferredDistance
            float radialError = dist - enemy->preferredDistance;
            glm::vec3 radialCorrection = toPlayerDir * radialError * 0.5f;

            glm::vec3 moveDir = tangent + radialCorrection;
            if (glm::dot(moveDir, moveDir) > 0.0001f) moveDir = glm::normalize(moveDir);

            // Periodically flip strafe direction for variety
            enemy->strafeFlipTimer -= deltaTime;
            if (enemy->strafeFlipTimer <= 0.0f) {
                enemy->strafeDirection *= -1;
                enemy->strafeFlipTimer = enemy->strafeFlipInterval;
            }

            // Single forward obstacle check — if blocked, reverse strafe immediately
            if (collision && glm::dot(moveDir, moveDir) > 0.0001f) {
                Ray ray;
                ray.origin = enemyPos + glm::vec3(0.0f, 1.0f, 0.0f);
                ray.direction = glm::normalize(moveDir);
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

        // ─── Facing ─────────────────────────────────────────────────────
        // Instantly snaps or smoothly rotates entity yaw toward a direction.
        static void faceDirection(our::Entity* entity, const glm::vec3& dir, float maxTurnStep) {
            if (maxTurnStep <= 0.0f || glm::dot(dir, dir) <= 0.000001f) return;

            float wanted = std::atan2(dir.x, dir.z);
            float& current = entity->localTransform.rotation.y;
            float delta = std::atan2(std::sin(wanted - current), std::cos(wanted - current));

            current += glm::clamp(delta, -maxTurnStep, maxTurnStep);
        }

        // ─── FSM transition ─────────────────────────────────────────────
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
                glm::vec3 toPlayerDir = (XZDistance > 0.0001f) ? (toPlayerXZ / XZDistance) : glm::vec3(0.0f);

                // ── FSM state transition ──
                updateState(enemy, distanceToPlayer);

                glm::vec3 movementDirection(0.0f);
                float movementSpeed = 0.0f;
                glm::vec3 faceDir = toPlayerDir;  // default: face the player

                using S = EnemyComponent::AIState;

                if (enemy->type == EnemyType::Flyer) {
                    // ────────────── FLYER ──────────────
                    switch (enemy->aiState) {
                        case S::Idle:
                            break;

                        case S::Aggro:
                            movementDirection = orbitStrafe(enemyPos, playerPos, enemy, deltaTime, collisionSystem);
                            movementSpeed = enemy->moveSpeed;
                            break;

                        case S::Attacking:
                            movementDirection = orbitStrafe(enemyPos, playerPos, enemy, deltaTime, collisionSystem);
                            movementSpeed = enemy->moveSpeed * 0.5f;

                            if (WeaponComponent* weapon = enemyEntity->getComponent<WeaponComponent>()) {
                                glm::vec3 muzzleWorld = glm::vec3(enemyEntity->getLocalToWorldMatrix() *
                                                                  glm::vec4(weapon->muzzleOffset, 1.0f));
                                ProjectileSystem::fire(world, app, enemyEntity, playerPos - muzzleWorld,
                                                       CollisionLayer::LAYER_ENEMY);
                            }
                            break;
                    }

                    // Flyer always faces the player (it's strafing, not turning away)
                    faceDir = toPlayerDir;

                    // Hover bob
                    if (!enemy->hoverOriginSet) {
                        enemy->hoverOriginY = enemyEntity->localTransform.position.y;
                        enemy->hoverOriginSet = true;
                    }
                    enemyEntity->localTransform.position.y =
                        enemy->hoverOriginY + enemy->baseHeight +
                        glm::sin(t * enemy->hoverFrequency) * enemy->hoverAmplitude;

                } else {
                    // ────────────── GROUND (Brute / Charger) ──────────────
                    switch (enemy->aiState) {
                        case S::Idle:
                            // Stand still
                            break;

                        case S::Aggro: {
                            bool dangerDetected = false;
                            glm::vec3 steerDir =
                                contextSteer(enemyPos, toPlayerDir, enemy->moveSpeed, collisionSystem, dangerDetected);

                            movementDirection = steerDir;
                            movementSpeed = enemy->moveSpeed;

                            if (dangerDetected) {
                                // Obstacle avoidance active: face the avoidance direction
                                // so steering is VISUALLY OBVIOUS (enemy turns sideways)
                                faceDir = steerDir;
                            } else {
                                // Clear path: face the player directly while walking
                                faceDir = toPlayerDir;
                            }
                            break;
                        }

                        case S::Attacking:
                            // Stop moving, face the player
                            faceDir = toPlayerDir;
                            break;
                    }

                    // ── Ground tracking (same approach as player movement) ──
                    // Raycast downward from slightly above feet to find ground surface,
                    // then snap the enemy's Y directly to it.
                    if (collisionSystem) {
                        Ray downRay;
                        downRay.origin = enemyPos + glm::vec3(0.0f, 0.5f, 0.0f);
                        downRay.direction = glm::vec3(0.0f, -1.0f, 0.0f);
                        HitInfo groundHit =
                            collisionSystem->raycast(downRay, 50.0f, CollisionLayer::LAYER_ENVIRONMENT);
                        if (groundHit.hit) {
                            enemyEntity->localTransform.position.y = groundHit.point.y;
                        }
                    }
                }

                // ── Apply facing ──
                faceDirection(enemyEntity, faceDir, enemy->turnSpeed * deltaTime);

                // ── Apply movement (XZ only — ground tracking handles Y) ──
                if (glm::dot(movementDirection, movementDirection) > 0.000001f && movementSpeed > 0.0f) {
                    glm::vec3 move = glm::normalize(movementDirection) * (movementSpeed * deltaTime);
                    enemyEntity->localTransform.position.x += move.x;
                    enemyEntity->localTransform.position.z += move.z;
                    // Y movement handled by hover (flyer) or ground tracking (ground) above
                }
            }
        }
    };

}  // namespace gameplay
