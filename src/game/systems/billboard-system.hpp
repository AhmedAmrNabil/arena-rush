#pragma once

#include <components/camera.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../components/billboard.hpp"
#include "../components/hit-marker.hpp"

namespace gameplay {

    class BillboardSystem {
    public:
        void update(our::World* world, our::Entity* cameraEntity) {
            if (!world || !cameraEntity) return;

            glm::vec3 cameraPos = glm::vec3(cameraEntity->getLocalToWorldMatrix()[3]);
            constexpr float referenceDistance = 8.0f;
            constexpr float minScaleFactor = 0.35f;
            constexpr float maxScaleFactor = 1.0f;

            for (our::Entity* entity : world->getEntities()) {
                if (!entity->getComponent<BillboardComponent>()) continue;

                glm::vec3 toCamera = cameraPos - entity->localTransform.position;
                float distance = glm::length(toCamera);
                if (distance <= 0.001f) continue;

                glm::quat faceCamera = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), toCamera / distance);
                entity->localTransform.rotation = glm::eulerAngles(faceCamera);

                if (HitMarkerComponent* marker = entity->getComponent<HitMarkerComponent>()) {
                    float t = glm::clamp(marker->age / marker->lifetime, 0.0f, 1.0f);
                    float baseScale = glm::mix(marker->startScale, marker->endScale, t);
                    float distanceScale = glm::clamp(distance / referenceDistance, minScaleFactor, maxScaleFactor);
                    entity->localTransform.scale = glm::vec3(baseScale * distanceScale);
                }
            }
        }
    };

}  // namespace gameplay
