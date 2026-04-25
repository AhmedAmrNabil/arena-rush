#pragma once

#include <cmath>
#include <components/model-renderer.hpp>
#include <ecs/transform.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

namespace gameplay {

    class WeaponVisualSystem {
        our::Entity* weaponEntity = nullptr;
        our::Transform baseWeaponTransform{};
        bool hasBaseWeaponTransform = false;

        float reloadElapsed = 1.0f;
        float reloadDuration = 1.0f;

        static float pulse(float t, float start, float end) {
            if (t <= start || t >= end) return 0.0f;
            float x = (t - start) / (end - start);
            return std::sin(glm::pi<float>() * x);
        }

        static our::Entity* findWeaponEntity(our::World* world) {
            for (our::Entity* entity : world->getEntities()) {
                our::ModelRendererComponent* renderer = entity->getComponent<our::ModelRendererComponent>();
                if (renderer && renderer->firstPersonOverlay) return entity;
            }
            return nullptr;
        }

    public:
        void destroy() {
            weaponEntity = nullptr;
            hasBaseWeaponTransform = false;
            reloadElapsed = reloadDuration;
        }

        void onReloadStart(float duration = 1.0f) {
            reloadDuration = glm::max(duration, 0.05f);
            reloadElapsed = 0.0f;
        }

        void update(our::World* world, float deltaTime, bool isAiming) {
            if (!world || deltaTime <= 0.0f) return;
            deltaTime = glm::min(deltaTime, 0.05f);

            our::Entity* foundWeaponEntity = findWeaponEntity(world);
            if (foundWeaponEntity != weaponEntity) {
                weaponEntity = foundWeaponEntity;
                hasBaseWeaponTransform = false;
            }

            if (weaponEntity && !hasBaseWeaponTransform) {
                baseWeaponTransform = weaponEntity->localTransform;
                hasBaseWeaponTransform = true;
            }

            if (!weaponEntity || !hasBaseWeaponTransform) return;

            glm::vec3 reloadPosition = glm::vec3(0.0f);
            glm::vec3 reloadRotation = glm::vec3(0.0f);
            if (reloadElapsed < reloadDuration) {
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

            our::Transform transform = baseWeaponTransform;
            transform.position += reloadPosition;
            transform.rotation += reloadRotation;
            weaponEntity->localTransform = transform;
        }
    };

}  // namespace gameplay
