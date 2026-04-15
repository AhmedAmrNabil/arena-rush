#pragma once

#include <algorithm>
#include <application.hpp>
#include <cmath>
#include <components/camera.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <json/json.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <vector>

#include "../components/collider.hpp"
#include "../components/enemy.hpp"
#include "../components/health.hpp"

namespace gameplay {

    class EnemyHealthBarSystem {
        struct Config {
            bool enabled = true;
            bool showWhenDamagedOnly = true;
            float maxDistance = 24.0f;
        } config;

        our::Mesh* rectangle = nullptr;
        our::ShaderProgram* shader = nullptr;
        our::TintedMaterial* material = nullptr;

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

        static our::CameraComponent* findCamera(our::World* world) {
            for (our::Entity* entity : world->getEntities())
                if (auto* camera = entity->getComponent<our::CameraComponent>(); camera) return camera;

            return nullptr;
        }

        static our::Vertex makeVertex(const glm::vec3& position, const glm::vec2& texCoord) {
            our::Vertex vertex{};
            vertex.position = position;
            vertex.color = {255, 255, 255, 255};
            vertex.tex_coord = texCoord;
            vertex.normal = {0.0f, 0.0f, 1.0f};
            return vertex;
        }

        void drawRect(const glm::mat4& projection, const glm::vec2& position, const glm::vec2& size,
                      const glm::vec4& tint) const {
            if (!(rectangle && material && shader) || size.x <= 0.0f || size.y <= 0.0f || tint.a <= 0.0f) return;

            material->tint = tint;
            material->setup();
            shader->set("transform", projection * glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) *
                                         glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f)));
            rectangle->draw();
        }

    public:
        void configure(const nlohmann::json& sceneConfig) {
            if (!(sceneConfig.contains("game") && sceneConfig["game"].contains("enemyHealthBars"))) return;

            const auto& healthBarConfig = sceneConfig["game"]["enemyHealthBars"];
            if (!healthBarConfig.is_object()) return;

            config = {};
            config.enabled = healthBarConfig.value("enabled", config.enabled);
            config.showWhenDamagedOnly = healthBarConfig.value("showWhenDamagedOnly", config.showWhenDamagedOnly);
            config.maxDistance = healthBarConfig.value("maxDistance", config.maxDistance);
        }

        void initialize() {
            shader = new our::ShaderProgram();
            shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
            shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
            shader->link();

            material = new our::TintedMaterial();
            material->shader = shader;
            material->pipelineState.blending.enabled = true;
            material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            material->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;
            material->pipelineState.depthTesting.enabled = false;
            material->pipelineState.faceCulling.enabled = false;
            material->pipelineState.depthMask = false;

            std::vector<our::Vertex> vertices = {
                makeVertex({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}),
                makeVertex({1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}),
                makeVertex({1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),
                makeVertex({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}),
            };
            std::vector<unsigned int> elements = {0, 1, 2, 2, 3, 0};
            rectangle = new our::Mesh(vertices, elements);
        }

        void render(our::World* world, our::Application* app) const {
            if (!(config.enabled && world && app && rectangle && material && shader)) return;

            our::CameraComponent* camera = findCamera(world);
            if (!camera) return;

            glm::ivec2 framebufferSize = app->getFrameBufferSize();
            if (framebufferSize.x <= 0 || framebufferSize.y <= 0) return;

            glm::mat4 view = camera->getViewMatrix();
            glm::mat4 proj = camera->getProjectionMatrix(framebufferSize);
            glm::mat4 viewProj = proj * view;
            glm::mat4 overlayProj = glm::ortho(0.0f, static_cast<float>(framebufferSize.x),
                                               static_cast<float>(framebufferSize.y), 0.0f, 1.0f, -1.0f);
            glm::vec3 cameraPos = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));

            for (our::Entity* entity : world->getEntities()) {
                auto* health = entity->getComponent<HealthComponent>();
                if (!(entity->getComponent<EnemyComponent>() && health) || health->isDead || health->maxHealth <= 0.0f)
                    continue;

                float healthRatio = glm::clamp(health->currentHealth / health->maxHealth, 0.0f, 1.0f);
                if (config.showWhenDamagedOnly && healthRatio >= 0.999f) continue;

                glm::mat4 localToWorld = entity->getLocalToWorldMatrix();
                glm::vec3 enemyPosition = glm::vec3(localToWorld * glm::vec4(0, 0, 0, 1));
                float distance = glm::distance(enemyPosition, cameraPos);
                if (distance > config.maxDistance) continue;

                ColliderComponent* collider = entity->getComponent<ColliderComponent>();
                float barHeightOffset = estimateBarHeight(entity, collider) + 0.35;

                glm::vec4 clipSpacePosition = viewProj * localToWorld * glm::vec4(0.0f, barHeightOffset, 0.0f, 1.0f);
                if (clipSpacePosition.w <= 0.0f) continue;

                glm::vec3 ndc = glm::vec3(clipSpacePosition) / clipSpacePosition.w;
                if (ndc.z < -1.0f || ndc.z > 1.0f || ndc.x < -1.1f || ndc.x > 1.1f || ndc.y < -1.1f || ndc.y > 1.1f)
                    continue;

                float dist = glm::clamp((distance - 6.0f) / glm::max(1.0f, config.maxDistance - 6.0f), 0.0f, 1.0f);
                float alpha = 1.0f - glm::smoothstep(config.maxDistance * 0.7f, config.maxDistance, distance);
                if (alpha <= 0.0f) continue;

                float barWidth = glm::mix(86.0f, 48.0f, dist);
                float barHeight = glm::mix(10.0f, 6.0f, dist);
                glm::vec2 screenCenter = {
                    (ndc.x * 0.5f + 0.5f) * framebufferSize.x,
                    (1.0f - (ndc.y * 0.5f + 0.5f)) * framebufferSize.y,
                };

                glm::vec2 barPosition = {screenCenter.x - barWidth * 0.5f, screenCenter.y - barHeight};
                glm::vec2 outerSize = {barWidth + 2.0f, barHeight + 2.0f};
                glm::vec2 innerPosition = barPosition + glm::vec2(2.0f, 2.0f);
                glm::vec2 innerSize = {barWidth - 4.0f, barHeight - 4.0f};
                glm::vec2 fillSize = {innerSize.x * healthRatio, innerSize.y};

                drawRect(overlayProj, barPosition - glm::vec2(1.0f), outerSize, {0.02f, 0.03f, 0.05f, 0.92f * alpha});
                drawRect(overlayProj, barPosition, {barWidth, barHeight}, {0.12f, 0.14f, 0.18f, 0.88f * alpha});

                glm::vec4 fillColor = healthColor(healthRatio);
                fillColor.a = 0.96f * alpha;
                drawRect(overlayProj, innerPosition, fillSize, fillColor);

                if (fillSize.x > 6.0f) {
                    drawRect(overlayProj, innerPosition + glm::vec2(0.0f, 1.0f),
                             {fillSize.x, std::max(1.0f, innerSize.y * 0.28f)}, {1.0f, 1.0f, 1.0f, 0.12f * alpha});
                }
            }
        }

        void destroy() {
            delete rectangle;
            delete material;
            delete shader;
            rectangle = nullptr;
            material = nullptr;
            shader = nullptr;
        }
    };

}  // namespace gameplay
