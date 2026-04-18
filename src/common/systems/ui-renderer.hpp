#pragma once

#include <glad/gl.h>

#include <components/camera.hpp>
#include <ecs/world.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vector>

#include "../asset-loader.hpp"
#include "../material/material.hpp"
#include "../mesh/mesh.hpp"
#include "../mesh/vertex.hpp"
#include "../shader/shader.hpp"

namespace our {

    struct ScreenPoint {
        glm::vec2 position;
        float depth;
        bool visible;
    };

    class UIRenderer {
        Mesh* quad = nullptr;
        TintedMaterial* material = nullptr;

        static Vertex makeVertex(const glm::vec3& position, const glm::vec2& texCoord) {
            Vertex vertex{};
            vertex.position = position;
            vertex.color = {255, 255, 255, 255};
            vertex.tex_coord = texCoord;
            vertex.normal = {0.0f, 0.0f, 1.0f};
            return vertex;
        }

    public:
        void initialize() {
            material = new TintedMaterial();
            material->shader = AssetLoader<ShaderProgram>::get("tinted");
            material->pipelineState.blending.enabled = true;
            material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            material->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;
            material->pipelineState.depthTesting.enabled = false;
            material->pipelineState.faceCulling.enabled = false;
            material->pipelineState.depthMask = false;

            std::vector<Vertex> vertices = {
                makeVertex({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}),
                makeVertex({1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}),
                makeVertex({1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),
                makeVertex({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}),
            };
            std::vector<unsigned int> elements = {0, 1, 2, 2, 3, 0};
            quad = new Mesh(vertices, elements);
        }

        void destroy() {
            delete quad;
            delete material;
            quad = nullptr;
            material = nullptr;
        }

        glm::mat4 overlayProjection(glm::ivec2 framebufferSize) const {
            return glm::ortho(0.0f, static_cast<float>(framebufferSize.x), static_cast<float>(framebufferSize.y), 0.0f,
                              1.0f, -1.0f);
        }

        void drawRect(const glm::mat4& projection, const glm::vec2& position, const glm::vec2& size,
                      const glm::vec4& color) const {
            if (!(quad && material && material->shader)) return;
            if (size.x <= 0.0f || size.y <= 0.0f || color.a <= 0.0f) return;

            material->tint = color;
            material->setup();
            material->shader->set("transform", projection * glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f)) *
                                                   glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f)));
            quad->draw();
        }

        static CameraComponent* findActiveCamera(World* world) {
            if (!world) return nullptr;

            for (Entity* entity : world->getEntities())
                if (auto* camera = entity->getComponent<CameraComponent>()) return camera;

            return nullptr;
        }

        static ScreenPoint worldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj,
                                         glm::ivec2 framebufferSize, float frustumMargin = 1.1f) {
            ScreenPoint result{{0.0f, 0.0f}, 0.0f, false};

            glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
            if (clip.w <= 0.0f) return result;  // behind camera

            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.z < -1.0f || ndc.z > 1.0f) return result;
            if (ndc.x < -frustumMargin || ndc.x > frustumMargin) return result;
            if (ndc.y < -frustumMargin || ndc.y > frustumMargin) return result;

            result.position = {
                (ndc.x * 0.5f + 0.5f) * framebufferSize.x,
                (1.0f - (ndc.y * 0.5f + 0.5f)) * framebufferSize.y,
            };

            result.depth = ndc.z;
            result.visible = true;
            return result;
        }
    };

}  // namespace our
