#pragma once

#include <algorithm>
#include <asset-loader.hpp>
#include <components/mesh-renderer.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

#include "../components/projectile-trail.hpp"
#include "../components/projectile.hpp"
#include "../components/trail-segment.hpp"

namespace gameplay {

    class TrailSystem {
        static our::Entity* spawnSegment(our::World* world, const glm::vec3& position, const std::string& materialName,
                                         const std::string& meshName, float lifetime, float startScale,
                                         float endScale) {
            if (!world || materialName.empty() || lifetime <= 0.0f) return nullptr;

            our::Mesh* mesh = our::AssetLoader<our::Mesh>::get(meshName);
            our::Material* material = our::AssetLoader<our::Material>::get(materialName);
            if (!mesh || !material) return nullptr;

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

            // update existing segments and remove expired ones
            for (our::Entity* entity : world->getEntities()) {
                TrailSegmentComponent* seg = entity->getComponent<TrailSegmentComponent>();
                if (!seg) continue;
                seg->age += deltaTime;
                float t = glm::clamp(seg->age / seg->lifetime, 0.0f, 1.0f);
                float scale = glm::mix(seg->startScale, seg->endScale, t);
                entity->localTransform.scale = glm::vec3(scale);
                if (seg->age >= seg->lifetime) world->markForRemoval(entity);
            }

            // spawn new segments for projectiles with trails
            struct SpawnRequest {
                glm::vec3 position;
                std::string material;
                std::string mesh;
                float lifetime;
                float headScale;
                float tailScale;
            };
            std::vector<SpawnRequest> spawns;

            for (our::Entity* entity : world->getEntities()) {
                ProjectileComponent* proj = entity->getComponent<ProjectileComponent>();
                if (!proj) continue;
                ProjectileTrailComponent* trail = entity->getComponent<ProjectileTrailComponent>();
                if (!trail || trail->maxSegments <= 0 || trail->material.empty()) continue;

                trail->spawnTimer -= deltaTime;
                if (trail->spawnTimer <= 0.0f) {
                    trail->spawnTimer = trail->segmentInterval();
                    spawns.push_back({entity->localTransform.position, trail->material, trail->mesh,
                                      trail->segmentLifetime, trail->headScale, trail->tailScale});
                }
            }

            for (const SpawnRequest& s : spawns) {
                spawnSegment(world, s.position, s.material, s.mesh, s.lifetime, s.headScale, s.tailScale);
            }
        }
    };

}  // namespace gameplay
