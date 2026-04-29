#pragma once

#include <cmath>
#include <components/camera.hpp>
#include <components/model-renderer.hpp>
#include <ecs/transform.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

namespace gameplay {

    class WeaponVisualSystem {
        our::Entity* weaponEntity = nullptr;
        our::Entity* cameraEntity = nullptr;
        our::Transform baseWeaponTransform{};
        bool hasBaseWeaponTransform = false;
        our::Entity* playerEntity = nullptr;

        glm::vec3 recoilPosition = glm::vec3(0.0f);
        glm::vec3 recoilPositionVelocity = glm::vec3(0.0f);
        glm::vec3 recoilRotation = glm::vec3(0.0f);
        glm::vec3 recoilRotationVelocity = glm::vec3(0.0f);

        glm::vec3 cameraRecoilRotation = glm::vec3(0.0f);
        glm::vec3 cameraRecoilRotationVelocity = glm::vec3(0.0f);
        glm::vec3 appliedCameraRecoilRotation = glm::vec3(0.0f);

        float reloadElapsed = 1.0f;
        float reloadDuration = 1.0f;

        float bobPhase = 0.0f;
        float bobStrength = 0.0f;
        unsigned int shotIndex = 0;

        static void integrateSpring(glm::vec3& value, glm::vec3& velocity, float stiffness, float damping,
                                    float deltaTime) {
            glm::vec3 acceleration = (-stiffness * value) - (damping * velocity);
            velocity += acceleration * deltaTime;
            value += velocity * deltaTime;
        }

        static float pulse(float t, float start, float end) {
            if (t <= start || t >= end) return 0.0f;
            float x = (t - start) / (end - start);
            return std::sin(glm::pi<float>() * x);
        }

        our::Entity* findWeaponEntity() {
            if (!playerEntity) return nullptr;
            PlayerComponent* playerComp = playerEntity->getComponent<PlayerComponent>();
            if (!playerComp || !playerComp->currentWeapon) return nullptr;
            return playerComp->currentWeapon->getOwner();
        }

        static our::Entity* findCameraEntity(our::World* world) {
            for (our::Entity* entity : world->getEntities())
                if (entity->getComponent<our::CameraComponent>()) return entity;
            return nullptr;
        }

    public:
        void initlize(our::Entity* playerEntity, our::Entity* cameraEntity) {
            this->playerEntity = playerEntity;
            this->cameraEntity = cameraEntity;
        }

        void destroy() {
            if (cameraEntity && glm::dot(appliedCameraRecoilRotation, appliedCameraRecoilRotation) > 0.0f)
                cameraEntity->localTransform.rotation -= appliedCameraRecoilRotation;
            weaponEntity = nullptr;
            cameraEntity = nullptr;
            hasBaseWeaponTransform = false;
            recoilPosition = glm::vec3(0.0f);
            recoilPositionVelocity = glm::vec3(0.0f);
            recoilRotation = glm::vec3(0.0f);
            recoilRotationVelocity = glm::vec3(0.0f);
            cameraRecoilRotation = glm::vec3(0.0f);
            cameraRecoilRotationVelocity = glm::vec3(0.0f);
            appliedCameraRecoilRotation = glm::vec3(0.0f);
            reloadElapsed = reloadDuration;
            bobPhase = 0.0f;
            bobStrength = 0.0f;
            shotIndex = 0;
        }

        void onFire() {
            float side = (shotIndex % 2 == 0 ? 1.0f : -1.0f) * (0.75f + 0.25f * std::sin(shotIndex * 1.73f));
            ++shotIndex;

            recoilPosition += glm::vec3(0.012f * side, 0.018f, 0.12f);
            recoilPositionVelocity += glm::vec3(0.08f * side, 0.16f, 1.35f);

            recoilRotation += glm::vec3(glm::radians(-3.2f), glm::radians(0.45f * side), glm::radians(1.5f * side));
            recoilRotationVelocity +=
                glm::vec3(glm::radians(-34.0f), glm::radians(4.0f * side), glm::radians(13.0f * side));

            cameraRecoilRotation +=
                glm::vec3(glm::radians(0.95f), glm::radians(0.20f * side), glm::radians(-0.16f * side));
            cameraRecoilRotationVelocity +=
                glm::vec3(glm::radians(10.0f), glm::radians(3.5f * side), glm::radians(-2.8f * side));
            cameraRecoilRotation = glm::clamp(cameraRecoilRotation,
                                              glm::vec3(glm::radians(-0.2f), glm::radians(-1.0f), glm::radians(-0.7f)),
                                              glm::vec3(glm::radians(3.0f), glm::radians(1.0f), glm::radians(0.7f)));
        }

        void onReloadStart(float duration = 1.0f) {
            reloadDuration = glm::max(duration, 0.05f);
            reloadElapsed = 0.0f;

            recoilPosition += glm::vec3(-0.015f, -0.018f, 0.035f);
            recoilPositionVelocity += glm::vec3(-0.11f, -0.22f, 0.42f);
            recoilRotation += glm::vec3(glm::radians(1.1f), 0.0f, glm::radians(-1.8f));
            recoilRotationVelocity += glm::vec3(glm::radians(8.0f), glm::radians(-4.0f), glm::radians(-11.0f));
        }

        void update(our::World* world, float deltaTime, const glm::vec3& playerVelocity, bool isGrounded,
                    bool isAiming) {
            if (!world || deltaTime <= 0.0f) return;
            deltaTime = glm::min(deltaTime, 0.05f);

            our::Entity* foundWeaponEntity = findWeaponEntity();
            if (foundWeaponEntity != weaponEntity) {
                weaponEntity = foundWeaponEntity;
                hasBaseWeaponTransform = false;
            }

            if (weaponEntity && !hasBaseWeaponTransform) {
                baseWeaponTransform = weaponEntity->localTransform;
                hasBaseWeaponTransform = true;
            }

            integrateSpring(recoilPosition, recoilPositionVelocity, 95.0f, 17.0f, deltaTime);
            integrateSpring(recoilRotation, recoilRotationVelocity, 110.0f, 19.0f, deltaTime);
            integrateSpring(cameraRecoilRotation, cameraRecoilRotationVelocity, 80.0f, 15.0f, deltaTime);

            if (cameraEntity) {
                glm::vec3 kickDelta = cameraRecoilRotation - appliedCameraRecoilRotation;
                float pitchLimit = glm::half_pi<float>() * 0.99f;
                cameraEntity->localTransform.rotation += kickDelta;
                cameraEntity->localTransform.rotation.x =
                    glm::clamp(cameraEntity->localTransform.rotation.x, -pitchLimit, pitchLimit);
                appliedCameraRecoilRotation = cameraRecoilRotation;
            } else {
                appliedCameraRecoilRotation = glm::vec3(0.0f);
            }

            if (!weaponEntity || !hasBaseWeaponTransform) return;

            glm::vec3 reloadPosition = glm::vec3(0.0f);
            glm::vec3 reloadRotation = glm::vec3(0.0f);
            bool isReloading = reloadElapsed < reloadDuration;
            if (isReloading) {
                reloadElapsed = glm::min(reloadElapsed + deltaTime, reloadDuration);
                float t = reloadElapsed / reloadDuration;
                float prep = pulse(t, 0.00f, 0.22f);
                float drop = pulse(t, 0.08f, 0.58f);
                float insert = pulse(t, 0.46f, 0.68f);
                float charge = pulse(t, 0.70f, 0.88f);
                float settle = pulse(t, 0.78f, 1.00f);
                float motionScale = isAiming ? 0.68f : 1.0f;

                reloadPosition = glm::vec3(-0.13f * drop + 0.045f * insert + 0.05f * charge,
                                           -0.50f * drop + 0.15f * insert + 0.035f * settle,
                                           0.22f * drop - 0.18f * insert + 0.10f * charge - 0.045f * settle) *
                                 motionScale;

                reloadRotation =
                    glm::vec3(glm::radians(19.0f) * drop - glm::radians(8.0f) * insert - glm::radians(5.0f) * charge +
                                  glm::radians(3.0f) * settle,
                              glm::radians(-8.0f) * drop + glm::radians(4.0f) * insert + glm::radians(3.5f) * charge,
                              glm::radians(6.0f) * prep + glm::radians(13.0f) * drop - glm::radians(15.0f) * insert -
                                  glm::radians(5.0f) * charge + glm::radians(3.0f) * settle) *
                    motionScale;
            }

            float horizontalSpeed = glm::length(glm::vec2(playerVelocity.x, playerVelocity.z));
            if (isGrounded) {
                float targetBob = glm::clamp(horizontalSpeed / 8.0f, 0.0f, 1.0f) * (isAiming ? 0.28f : 1.0f);
                if (isReloading) targetBob *= 0.35f;
                bobStrength = glm::mix(bobStrength, targetBob, glm::clamp(10.0f * deltaTime, 0.0f, 1.0f));
                bobPhase += deltaTime * (6.5f + horizontalSpeed * 0.45f);
            } else {
                bobStrength = 0.0f;
            }

            glm::vec3 bobPosition =
                glm::vec3(std::sin(bobPhase) * 0.026f, std::abs(std::cos(bobPhase)) * 0.018f - 0.009f,
                          std::sin(bobPhase * 2.0f) * 0.010f) *
                bobStrength;
            glm::vec3 bobRotation =
                glm::vec3(std::cos(bobPhase) * glm::radians(0.75f), std::sin(bobPhase * 0.5f) * glm::radians(0.65f),
                          std::sin(bobPhase) * glm::radians(1.35f)) *
                bobStrength;

            our::Transform transform = baseWeaponTransform;
            transform.position += recoilPosition + reloadPosition + bobPosition;
            transform.rotation += recoilRotation + reloadRotation + bobRotation;
            weaponEntity->localTransform = transform;
        }
    };

}  // namespace gameplay
