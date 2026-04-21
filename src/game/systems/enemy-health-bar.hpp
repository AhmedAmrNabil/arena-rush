#pragma once

#include <algorithm>
#include <application.hpp>
#include <cmath>
#include <components/mesh-renderer.hpp>
#include <components/model-renderer.hpp>
#include <deserialize-utils.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <json/json.hpp>
#include <limits>
#include <systems/ui-renderer.hpp>

#include "../components/collider.hpp"
#include "../components/enemy.hpp"
#include "../components/health.hpp"

namespace gameplay {

    class EnemyHealthBarSystem {
        struct Config {
            bool enabled = true;
            float maxDistance = 24.0f;
            glm::vec4 lowColor = {0.84f, 0.18f, 0.24f, 0.96f};
            glm::vec4 midColor = {0.96f, 0.76f, 0.25f, 0.96f};
            glm::vec4 highColor = {0.28f, 0.84f, 0.45f, 0.96f};
            glm::vec4 borderColor = {0.02f, 0.03f, 0.05f, 0.92f};
            glm::vec4 backgroundColor = {0.12f, 0.14f, 0.18f, 0.88f};
            glm::vec4 highlightColor = {1.0f, 1.0f, 1.0f, 0.12f};
        } config;

        struct LocalBounds {
            glm::vec3 min = glm::vec3(0.0f);
            glm::vec3 max = glm::vec3(0.0f);
            bool valid = false;
        };

        static void deserializeColor(const nlohmann::json& colorConfig, const char* key, glm::vec4& color) {
            if (!(colorConfig.contains(key) && colorConfig[key].is_array())) return;

            const auto& value = colorConfig[key];
            if (value.size() == 3) {
                color = glm::vec4(value.get<glm::vec3>(), color.a);
            } else if (value.size() == 4) {
                color = value.get<glm::vec4>();
            }
        }

        static float estimateBarHeight(our::Entity* entity, const ColliderComponent* collider) {
            float scaleHeight = std::max(std::abs(entity->localTransform.scale.y) * 1.2f, 1.0f);
            if (!collider) return scaleHeight;

            float colliderHeight = collider->radius * 2.0f;
            if (collider->shape == ColliderShape::Capsule) colliderHeight = std::max(colliderHeight, collider->height);

            return std::max(colliderHeight, scaleHeight);
        }

        static LocalBounds computeBoundsFromMesh(const our::Mesh* mesh) {
            LocalBounds bounds;
            if (!mesh) return bounds;

            const auto& vertices = mesh->getVertices();
            if (vertices.empty()) return bounds;

            glm::vec3 minPoint(std::numeric_limits<float>::max());
            glm::vec3 maxPoint(std::numeric_limits<float>::lowest());
            for (const auto& vertex : vertices) {
                minPoint = glm::min(minPoint, vertex.position);
                maxPoint = glm::max(maxPoint, vertex.position);
            }

            bounds.min = minPoint;
            bounds.max = maxPoint;
            bounds.valid = true;
            return bounds;
        }

        static LocalBounds getVisualBounds(our::Entity* entity, const ColliderComponent* collider) {
            if (!entity) return {};

            if (auto* modelRenderer = entity->getComponent<our::ModelRendererComponent>();
                modelRenderer && modelRenderer->model) {
                LocalBounds bounds = computeBoundsFromMesh(modelRenderer->model->getCombinedMesh());
                if (bounds.valid) return bounds;
            }

            if (auto* meshRenderer = entity->getComponent<our::MeshRendererComponent>(); meshRenderer) {
                LocalBounds bounds = computeBoundsFromMesh(meshRenderer->mesh);
                if (bounds.valid) return bounds;
            }

            if (collider && collider->mesh) {
                LocalBounds bounds = computeBoundsFromMesh(collider->mesh);
                if (bounds.valid) return bounds;
            }

            return {};
        }

        static glm::vec3 estimateBarLocalAnchor(our::Entity* entity, const ColliderComponent* collider) {
            LocalBounds bounds = getVisualBounds(entity, collider);
            if (bounds.valid) {
                float height = std::max(bounds.max.y - bounds.min.y, 0.0f);
                float verticalPadding = std::max(height * 0.08f, 0.15f);
                return {
                    (bounds.min.x + bounds.max.x) * 0.5f,
                    bounds.max.y + verticalPadding,
                    (bounds.min.z + bounds.max.z) * 0.5f,
                };
            }

            return {0.0f, estimateBarHeight(entity, collider) + 0.35f, 0.0f};
        }

    public:
        void deserialize(const nlohmann::json& sceneConfig) {
            if (!(sceneConfig.contains("game") && sceneConfig["game"].contains("enemyHealthBars"))) return;

            const auto& healthBarConfig = sceneConfig["game"]["enemyHealthBars"];
            if (!healthBarConfig.is_object()) return;

            config = {};
            config.enabled = healthBarConfig.value("enabled", config.enabled);
            config.maxDistance = healthBarConfig.value("maxDistance", config.maxDistance);
            config.maxDistance = std::max(0.0f, config.maxDistance);

            if (healthBarConfig.contains("colors") && healthBarConfig["colors"].is_object()) {
                const auto& colors = healthBarConfig["colors"];
                deserializeColor(colors, "low", config.lowColor);
                deserializeColor(colors, "mid", config.midColor);
                deserializeColor(colors, "high", config.highColor);
                deserializeColor(colors, "border", config.borderColor);
                deserializeColor(colors, "background", config.backgroundColor);
                deserializeColor(colors, "highlight", config.highlightColor);
            }
        }

        void render(our::World* world, our::Application* app, const our::UIRenderer& ui,
                    const our::CameraComponent* camera) const {
            if (!(config.enabled && world && app && camera)) return;

            glm::ivec2 framebufferSize = app->getFrameBufferSize();
            if (framebufferSize.x <= 0 || framebufferSize.y <= 0) return;

            glm::mat4 view = camera->getViewMatrix();
            glm::mat4 proj = camera->getProjectionMatrix(framebufferSize);
            glm::mat4 viewProj = proj * view;
            glm::mat4 overlayProj = ui.overlayProjection(framebufferSize);
            glm::vec3 cameraPos = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));

            for (our::Entity* entity : world->getEntities()) {
                auto* health = entity->getComponent<HealthComponent>();
                if (!(entity->getComponent<EnemyComponent>() && health) || health->isDead || health->maxHealth <= 0.0f)
                    continue;

                float healthRatio = health->getHealthRatio();
                glm::mat4 localToWorld = entity->getLocalToWorldMatrix();
                glm::vec3 enemyPosition = glm::vec3(localToWorld * glm::vec4(0, 0, 0, 1));
                float distance = glm::distance(enemyPosition, cameraPos);

                bool visibleWhenDamaged = health->damageRevealTimer > 0.0f;
                bool visibleWhenClose = distance <= config.maxDistance;

                if (!(visibleWhenDamaged || visibleWhenClose)) continue;

                ColliderComponent* collider = entity->getComponent<ColliderComponent>();
                glm::vec3 barLocalAnchor = estimateBarLocalAnchor(entity, collider);
                glm::vec3 barWorldPos = glm::vec3(localToWorld * glm::vec4(barLocalAnchor, 1.0f));
                our::ScreenPoint screen = our::UIRenderer::worldToScreen(barWorldPos, viewProj, framebufferSize);
                if (!screen.visible) continue;

                float dist = glm::clamp((distance - 6.0f) / glm::max(1.0f, config.maxDistance - 6.0f), 0.0f, 1.0f);
                float alpha = visibleWhenDamaged
                                  ? 1.0f
                                  : 1.0f - glm::smoothstep(config.maxDistance * 0.7f, config.maxDistance, distance);
                if (alpha <= 0.0f) continue;

                float barWidth = glm::mix(86.0f, 48.0f, dist);
                float barHeight = glm::mix(10.0f, 6.0f, dist);

                glm::vec2 barPosition = {screen.position.x - barWidth * 0.5f, screen.position.y - barHeight};
                glm::vec2 outerSize = {barWidth + 2.0f, barHeight + 2.0f};
                glm::vec2 innerPosition = barPosition + glm::vec2(2.0f, 2.0f);
                glm::vec2 innerSize = {barWidth - 4.0f, barHeight - 4.0f};
                glm::vec2 fillSize = {innerSize.x * healthRatio, innerSize.y};

                glm::vec4 borderColor = config.borderColor;
                borderColor.a *= alpha;
                ui.drawRect(overlayProj, barPosition - glm::vec2(1.0f), outerSize, borderColor);

                glm::vec4 backgroundColor = config.backgroundColor;
                backgroundColor.a *= alpha;
                ui.drawRect(overlayProj, barPosition, {barWidth, barHeight}, backgroundColor);

                float clampedHealthRatio = glm::clamp(healthRatio, 0.0f, 1.0f);
                glm::vec4 fillColor =
                    clampedHealthRatio < 0.5f
                        ? glm::mix(config.lowColor, config.midColor, clampedHealthRatio * 2.0f)
                        : glm::mix(config.midColor, config.highColor, (clampedHealthRatio - 0.5f) * 2.0f);
                fillColor.a *= alpha;
                ui.drawRect(overlayProj, innerPosition, fillSize, fillColor);

                if (fillSize.x > 6.0f) {
                    glm::vec4 highlightColor = config.highlightColor;
                    highlightColor.a *= alpha;
                    ui.drawRect(overlayProj, innerPosition + glm::vec2(0.0f, 1.0f),
                                {fillSize.x, std::max(1.0f, innerSize.y * 0.28f)}, highlightColor);
                }
            }
        }
    };

}  // namespace gameplay
