#pragma once

#include <asset-loader.hpp>
#include <components/mesh-renderer.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../components/billboard.hpp"
#include "../components/hit-marker.hpp"

namespace gameplay {

    class HitMarkerSystem {
        static our::TintedMaterial* cloneTintedMaterial(const std::string& materialName) {
            our::Material* base = our::AssetLoader<our::Material>::get(materialName);
            auto* baseTinted = dynamic_cast<our::TintedMaterial*>(base);
            if (!baseTinted) return nullptr;

            auto* clone = new our::TintedMaterial();
            clone->pipelineState = baseTinted->pipelineState;
            clone->shader = baseTinted->shader;
            clone->transparent = baseTinted->transparent;
            clone->tint = baseTinted->tint;
            return clone;
        }

        static our::Entity* spawnMarkerEntity(our::World* world, const glm::vec3& position,
                                              our::TintedMaterial* material, const HitMarkerComponent& settings) {
            if (!world || !material) return nullptr;

            our::Mesh* mesh = our::AssetLoader<our::Mesh>::get("plane");
            if (!mesh) return nullptr;

            our::Entity* marker = world->add();
            marker->name = "HitMarker";
            marker->localTransform.position = position;
            marker->localTransform.scale = glm::vec3(settings.startScale);

            our::MeshRendererComponent* renderer = marker->addComponent<our::MeshRendererComponent>();
            renderer->mesh = mesh;
            renderer->material = material;
            renderer->transform = glm::mat4(1.0f);

            HitMarkerComponent* markerComp = marker->addComponent<HitMarkerComponent>();
            *markerComp = settings;
            markerComp->ownedMaterial = material;

            marker->addComponent<BillboardComponent>();

            return marker;
        }

    public:
        static void spawn(our::World* world, const glm::vec3& point) {
            if (!world) return;

            HitMarkerComponent settings;
            settings.lifetime = 0.6f;  // make blood splatter last for 1.5 seconds instead of 0.3s

            // each marker needs its own material instance for independent alpha fade
            our::TintedMaterial* markerMaterial = cloneTintedMaterial("blood-splatter-material");
            if (!markerMaterial) return;

            our::Entity* marker = spawnMarkerEntity(world, point, markerMaterial, settings);
            if (!marker) {
                delete markerMaterial;
            }
        }

        static void update(our::World* world, float deltaTime) {
            if (!world || deltaTime <= 0.0f) return;

            for (our::Entity* entity : world->getEntities()) {
                HitMarkerComponent* marker = entity->getComponent<HitMarkerComponent>();
                if (!marker) continue;

                marker->age += deltaTime;
                float t = glm::clamp(marker->age / marker->lifetime, 0.0f, 1.0f);

                if (marker->ownedMaterial) {
                    if (auto* tinted = dynamic_cast<our::TintedMaterial*>(marker->ownedMaterial)) {
                        glm::vec4 tint = tinted->tint;
                        tint.a = glm::mix(marker->startAlpha, marker->endAlpha, t);
                        tinted->tint = tint;
                    }
                }

                if (marker->age >= marker->lifetime) {
                    if (marker->ownedMaterial) {
                        delete marker->ownedMaterial;
                        marker->ownedMaterial = nullptr;
                    }
                    world->markForRemoval(entity);
                }
            }
        }
    };

}  // namespace gameplay
