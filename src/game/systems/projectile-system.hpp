#pragma once

#include <GLFW/glfw3.h>

#include <algorithm>
#include <application.hpp>
#include <asset-loader.hpp>
#include <audio/audio-buffer.hpp>
#include <components/mesh-renderer.hpp>
#include <ecs/world.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "../components/collider.hpp"
#include "../components/health.hpp"
#include "../components/projectile-trail.hpp"
#include "../components/projectile.hpp"
#include "../components/weapon.hpp"
#include "collision-system.hpp"
#include "components/player.hpp"
#include "hit-marker-system.hpp"
#include "trail-system.hpp"

namespace gameplay {

    class ProjectileSystem {
        static our::Entity* spawnBullet(our::World* world, const WeaponComponent& weapon, const glm::vec3& origin,
                                        const glm::vec3& direction, CollisionLayer shooterLayer) {
            our::Mesh* mesh = our::AssetLoader<our::Mesh>::get("sphere");
            const char* materialName =
                shooterLayer == CollisionLayer::LAYER_PLAYER ? "projectile-player" : "projectile-enemy";
            our::Material* material = our::AssetLoader<our::Material>::get(materialName);
            if (!mesh || !material) return nullptr;

            our::Entity* bullet = world->add();
            bullet->name = "Projectile";
            bullet->localTransform.position = origin;
            bullet->localTransform.scale = glm::vec3(weapon.bulletScale);

            our::MeshRendererComponent* renderer = bullet->addComponent<our::MeshRendererComponent>();
            renderer->mesh = mesh;
            renderer->material = material;
            renderer->transform = glm::mat4(1.0f);

            ColliderComponent* collider = bullet->addComponent<ColliderComponent>();
            collider->shape = ColliderShape::Sphere;
            collider->layer = CollisionLayer::LAYER_PROJECTILE;
            collider->radius = weapon.bulletColliderRadius;
            collider->isTrigger = true;

            ProjectileComponent* proj = bullet->addComponent<ProjectileComponent>();
            proj->velocity = direction * weapon.bulletSpeed;
            proj->damage = weapon.bulletDamage;
            proj->lifetime = weapon.bulletLifetime;
            proj->shooterLayer = shooterLayer;

            if (weapon.trailEnabled && weapon.trailMaxSegments > 0) {
                ProjectileTrailComponent* trail = bullet->addComponent<ProjectileTrailComponent>();
                trail->material = materialName;
                trail->maxSegments = weapon.trailMaxSegments;
                trail->segmentLifetime = weapon.trailSegmentLifetime;
                trail->headScale = weapon.trailHeadScale;
                trail->tailScale = weapon.trailTailScale;
                trail->spawnTimer = 0.0f;
            }

            return bullet;
        }

    public:
        static void tickWeaponCooldowns(our::World* world, float deltaTime) {
            if (!world || deltaTime <= 0.0f) return;

            for (our::Entity* entity : world->getEntities())
                if (WeaponComponent* weapon = entity->getComponent<WeaponComponent>())
                    weapon->timer = std::max(0.0f, weapon->timer - deltaTime);
        }

        static bool fire(our::World* world, our::Application* app, our::Entity* shooter, const glm::vec3& aim,
                         CollisionLayer shooterLayer, WeaponComponent* weaponOverride = nullptr) {
            if (!world || !app || !shooter) return false;

            WeaponComponent* weapon = weaponOverride ? weaponOverride : shooter->getComponent<WeaponComponent>();
            if (!weapon || weapon->timer > 0.0f || glm::dot(aim, aim) <= 0.000001f) return false;

            glm::vec3 direction = glm::normalize(aim);
            glm::vec3 origin = glm::vec3(shooter->getLocalToWorldMatrix() * glm::vec4(weapon->muzzleOffset, 1.0f));

            if (!spawnBullet(world, *weapon, origin, direction, shooterLayer)) return false;

            weapon->timer = weapon->cooldown;

            if (!weapon->fireSound.empty())
                if (our::AudioBuffer* buffer = our::AssetLoader<our::AudioBuffer>::get(weapon->fireSound)) {
                    if (shooterLayer == CollisionLayer::LAYER_PLAYER)
                        app->getAudioSystem().playSound2D(buffer, 1.0f, 1.0f, false);
                    else
                        app->getAudioSystem().playSound(buffer, origin, 0.5f, 1.0f, false, 6.0f, 60.0f);
                }

            return true;
        }

        static bool handlePlayerReload(our::Application* app, our::Entity* playerEntity) {
            if (!app || !playerEntity) return false;

            PlayerComponent* playerComp = playerEntity->getComponent<PlayerComponent>();
            WeaponComponent* weapon = playerComp->currentWeapon;
            if (!weapon || !playerComp) return false;
            if (weapon->timer > 0.0f) return false;  // still in cooldown / already reloading

            bool wantsReload = app->getKeyboard().justPressed(GLFW_KEY_R) && weapon->currentAmmo < weapon->magSize;
            bool needsAutoReload = weapon->currentAmmo <= 0;

            if ((wantsReload || needsAutoReload) && weapon->maxAmmo > 0) {
                int reloadAmt = std::min(weapon->magSize - weapon->currentAmmo, weapon->maxAmmo);
                weapon->currentAmmo += reloadAmt;
                weapon->maxAmmo -= reloadAmt;
                weapon->timer = weapon->reloadTime;

                if (!weapon->reloadSound.empty())
                    if (our::AudioBuffer* buffer = our::AssetLoader<our::AudioBuffer>::get(weapon->reloadSound))
                        app->getAudioSystem().playSound2D(buffer, 1.0f, 1.0f, false);
                return true;
            }
            return false;
        }

        static bool handlePlayerFire(our::World* world, our::Application* app, const CollisionSystem& collisions,
                                     our::Entity* playerEntity) {
            if (!world || !app || !playerEntity) return false;

            PlayerComponent* playerComp = playerEntity->getComponent<PlayerComponent>();
            if (!playerComp) return false;

            WeaponComponent* weapon = playerComp->currentWeapon;
            if (!weapon) return false;

            if (weapon->automatic) {
                if (!app->getMouse().isPressed(GLFW_MOUSE_BUTTON_LEFT) && !app->getJoystick().isTriggerPressed(1))
                    return false;
            } else {
                if (!app->getMouse().justPressed(GLFW_MOUSE_BUTTON_LEFT) && !app->getJoystick().justPressedTrigger(1))
                    return false;
            }

            glm::mat4 playerM = playerEntity->getLocalToWorldMatrix();
            glm::vec3 cameraPos = glm::vec3(playerM[3]);
            glm::vec3 forward = glm::normalize(glm::vec3(playerM * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

            constexpr float aimDistance = 500.0f;
            HitInfo aimHit = collisions.raycast(Ray{cameraPos, forward}, aimDistance,
                                                CollisionLayer::LAYER_ENVIRONMENT | CollisionLayer::LAYER_ENEMY);
            glm::vec3 aimPoint = aimHit.hit ? aimHit.point : (cameraPos + forward * aimDistance);

            glm::vec3 muzzleOrigin =
                glm::vec3(playerM * glm::vec4(weapon ? weapon->muzzleOffset : glm::vec3(0.0f), 1.0f));
            glm::vec3 aimVector = aimPoint - muzzleOrigin;
            if (glm::dot(aimVector, aimVector) <= 0.000001f) aimVector = forward;

            bool fired = fire(world, app, playerEntity, aimVector, CollisionLayer::LAYER_PLAYER, weapon);
            if (fired) weapon->currentAmmo = std::max(0, weapon->currentAmmo - 1);
            return fired;
        }

        static void update(our::World* world, const CollisionSystem& collisions, float deltaTime) {
            if (!world || deltaTime <= 0.0f) return;

            for (our::Entity* entity : world->getEntities()) {
                ProjectileComponent* proj = entity->getComponent<ProjectileComponent>();
                if (!proj) continue;

                proj->lifetime -= deltaTime;
                if (proj->lifetime <= 0.0f) {
                    world->markForRemoval(entity);
                    continue;
                }

                glm::vec3 oldPos = entity->localTransform.position;
                glm::vec3 movement = proj->velocity * deltaTime;
                float moveDistance = glm::length(movement);

                if (moveDistance > 0.0001f) {
                    glm::vec3 dir = movement / moveDistance;

                    short hitMask =
                        getMaskForLayer(proj->shooterLayer) & ~CollisionLayer::LAYER_PROJECTILE & ~proj->shooterLayer;

                    ColliderComponent* bulletCollider = entity->getComponent<ColliderComponent>();
                    float extra = bulletCollider ? bulletCollider->radius : 0.0f;

                    HitInfo hit = collisions.raycast(Ray{oldPos, dir}, moveDistance + extra, hitMask);

                    if (hit.hit && hit.entity) {
                        HealthComponent* health = hit.entity->getComponent<HealthComponent>();
                        if (health && !health->isDead) {
                            health->takeDamage(proj->damage);
                            if (proj->shooterLayer == CollisionLayer::LAYER_PLAYER) {
                                HitMarkerSystem::spawn(world, hit.point);
                            }
                        }

                        const char* sparkMaterial = proj->shooterLayer == CollisionLayer::LAYER_PLAYER
                                                        ? "projectile-player"
                                                        : "projectile-enemy";
                        TrailSystem::spawnImpactSpark(world, hit.point, hit.normal, sparkMaterial);

                        entity->localTransform.position = hit.point;
                        proj->lifetime = 0.0f;
                        world->markForRemoval(entity);
                        continue;
                    }
                }

                entity->localTransform.position += movement;
            }
        }
    };

}  // namespace gameplay
