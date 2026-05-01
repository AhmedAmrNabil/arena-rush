#pragma once

#include <algorithm>
#include <asset-loader.hpp>
#include <components/mesh-renderer.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>

#include "../components/projectile-trail.hpp"
#include "../components/projectile.hpp"
#include "../components/trail-segment.hpp"

namespace gameplay {

    class TrailSystem {
        struct SpawnRequest {
            glm::vec3 position;
            std::string material;
            std::string mesh;
            float lifetime;
            float headScale;
            float tailScale;
        };

        static inline std::vector<SpawnRequest> spawnRequests;

        static our::Entity* spawnSegment(our::World* world, const glm::vec3& position, our::Material* material,
                                         const std::string& meshName, float lifetime, float startScale,
                                         float endScale) {
            if (!world || !material || lifetime <= 0.0f) return nullptr;

            our::Mesh* mesh = our::AssetLoader<our::Mesh>::get(meshName);
            if (!mesh) return nullptr;

            our::Entity* segment = world->add();
            segment->name = "TrailSegment";
            segment->localTransform.position = position;
            segment->localTransform.scale = glm::vec3(startScale);

            our::MeshRendererComponent* renderer = segment->addComponent<our::MeshRendererComponent>();
            renderer->mesh = mesh;
            renderer->material = material;
            renderer->transform = glm::mat4(1.0f);

            TrailSegmentComponent* seg = segment->addComponent<TrailSegmentComponent>();
            seg->age = 0.0f;
            seg->lifetime = lifetime;
            seg->startScale = startScale;
            seg->endScale = endScale;

            return segment;
        }

        static our::Entity* spawnSegment(our::World* world, const glm::vec3& position, const std::string& materialName,
                                         const std::string& meshName, float lifetime, float startScale,
                                         float endScale) {
            if (!world || materialName.empty() || lifetime <= 0.0f) return nullptr;

            our::Material* material = our::AssetLoader<our::Material>::get(materialName);
            if (!material) return nullptr;

            return spawnSegment(world, position, material, meshName, lifetime, startScale, endScale);
        }

    public:
        static void spawnImpactSpark(our::World* world, const glm::vec3& point, const glm::vec3& /*normal*/,
                                     const std::string& materialName) {
            constexpr float sparkLifetime = 0.12f;
            constexpr float sparkStart = 0.35f;
            constexpr float sparkEnd = 0.0f;
            spawnSegment(world, point, materialName, "sphere", sparkLifetime, sparkStart, sparkEnd);
        }

        static void update(our::World* world, float deltaTime) {
            if (!world || deltaTime <= 0.0f) return;

            spawnRequests.clear();

            for (our::Entity* entity : world->getEntities()) {
                TrailSegmentComponent* seg = entity->getComponent<TrailSegmentComponent>();
                if (seg) {
                    seg->age += deltaTime;
                    float t = glm::clamp(seg->age / seg->lifetime, 0.0f, 1.0f);
                    float scale = glm::mix(seg->startScale, seg->endScale, t);
                    entity->localTransform.scale = glm::vec3(scale);
                    if (seg->ownedMaterial) {
                        if (auto* tinted = dynamic_cast<our::TintedMaterial*>(seg->ownedMaterial)) {
                            glm::vec4 tint = tinted->tint;
                            tint.a = glm::mix(seg->startAlpha, seg->endAlpha, t);
                            tinted->tint = tint;
                        }
                    }
                    if (seg->age >= seg->lifetime) {
                        if (seg->ownedMaterial) {
                            delete seg->ownedMaterial;
                            seg->ownedMaterial = nullptr;
                        }
                        world->markForRemoval(entity);
                    }
                }

                if (!entity->getComponent<ProjectileComponent>()) continue;
                ProjectileTrailComponent* trail = entity->getComponent<ProjectileTrailComponent>();
                if (!trail || trail->maxSegments <= 0 || trail->material.empty()) continue;

                trail->spawnTimer -= deltaTime;
                if (trail->spawnTimer <= 0.0f) {
                    trail->spawnTimer = trail->segmentInterval();
                    spawnRequests.push_back({entity->localTransform.position, trail->material, trail->mesh,
                                             trail->segmentLifetime, trail->headScale, trail->tailScale});
                }
            }

            for (const SpawnRequest& s : spawnRequests) {
                spawnSegment(world, s.position, s.material, s.mesh, s.lifetime, s.headScale, s.tailScale);
            }
        }
    };

}  // namespace gameplay
