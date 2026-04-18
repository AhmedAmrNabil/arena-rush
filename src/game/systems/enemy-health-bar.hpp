#pragma once

#include <algorithm>
#include <application.hpp>
#include <cmath>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <json/json.hpp>
#include <systems/ui-renderer.hpp>

#include "../components/collider.hpp"
#include "../components/enemy.hpp"
#include "../components/health.hpp"

namespace gameplay {

    class EnemyHealthBarSystem {
        struct Config {
            bool enabled = true;
            float maxDistance = 24.0f;
        } config;

        static glm::vec4 healthColor(float ratio) {
            ratio = glm::clamp(ratio, 0.0f, 1.0f);
            glm::vec3 low = {0.84f, 0.18f, 0.24f};
            glm::vec3 mid = {0.96f, 0.76f, 0.25f};
            glm::vec3 high = {0.28f, 0.84f, 0.45f};
            glm::vec3 color =
                ratio < 0.5f ? glm::mix(low, mid, ratio * 2.0f) : glm::mix(mid, high, (ratio - 0.5f) * 2.0f);

            return glm::vec4(color, 1.0f);
        }

        static float estimateBarHeight(const our::Entity* entity, const ColliderComponent* collider) {
            float scaleHeight = std::max(std::abs(entity->localTransform.scale.y) * 1.2f, 1.0f);
            if (!collider) return scaleHeight;

            float colliderHeight = collider->radius * 2.0f;
            if (collider->shape == ColliderShape::Capsule) colliderHeight = std::max(colliderHeight, collider->height);

            return std::max(colliderHeight, scaleHeight);
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
        }

        void render(our::World* world, our::Application* app, const our::UIRenderer& ui) const {
            if (!(config.enabled && world && app)) return;

            our::CameraComponent* camera = our::UIRenderer::findActiveCamera(world);
            if (!camera) return;

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
                bool visibleWhenClose = distance <= 8.0f;

                if (!visibleWhenDamaged && distance > config.maxDistance) continue;
                if (!(visibleWhenDamaged || visibleWhenClose)) continue;

                ColliderComponent* collider = entity->getComponent<ColliderComponent>();
                float barHeightOffset = estimateBarHeight(entity, collider) + 0.35f;

                glm::vec3 barWorldPos = glm::vec3(localToWorld * glm::vec4(0.0f, barHeightOffset, 0.0f, 1.0f));
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

                ui.drawRect(overlayProj, barPosition - glm::vec2(1.0f), outerSize,
                            {0.02f, 0.03f, 0.05f, 0.92f * alpha});
                ui.drawRect(overlayProj, barPosition, {barWidth, barHeight}, {0.12f, 0.14f, 0.18f, 0.88f * alpha});

                glm::vec4 fillColor = healthColor(healthRatio);
                fillColor.a = 0.96f * alpha;
                ui.drawRect(overlayProj, innerPosition, fillSize, fillColor);

                if (fillSize.x > 6.0f) {
                    ui.drawRect(overlayProj, innerPosition + glm::vec2(0.0f, 1.0f),
                                {fillSize.x, std::max(1.0f, innerSize.y * 0.28f)}, {1.0f, 1.0f, 1.0f, 0.12f * alpha});
                }
            }
        }
    };

}  // namespace gameplay
