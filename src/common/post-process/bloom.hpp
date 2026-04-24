#pragma once

#include <glad/gl.h>

#include <glm/vec2.hpp>
#include <json/json.hpp>

#include "../shader/shader.hpp"
#include "../texture/sampler.hpp"
#include "framebuffer.hpp"

namespace our {

    class BloomPostProcess {
        void setupFullscreenState() const;
        void drawFullscreen(GLuint fullscreenVao) const;

        glm::ivec2 windowSize = glm::ivec2(0, 0);
        float threshold = 1.0f;
        float softKnee = 0.0f;
        float radius = 1.0f;
        float intensity = 1.0f;
        float blurHorizontal = 1.0f;
        float blurVertical = 1.0f;

        Framebuffer* sceneFramebuffer = nullptr;
        Texture2D* sceneBrightTarget = nullptr;
        Framebuffer* blurPingFramebuffer = nullptr;
        Framebuffer* blurPongFramebuffer = nullptr;

        ShaderProgram* blurShader = nullptr;
        ShaderProgram* combineShader = nullptr;
        Sampler* sampler = nullptr;

    public:
        void initialize(glm::ivec2 windowSize, const nlohmann::json& config);
        void destroy();
        void resize(glm::ivec2 windowSize);

        void bindSceneFramebuffer() const;

        Framebuffer* getSceneFramebuffer() const {
            return sceneFramebuffer;
        }

        Texture2D* getSceneColorTarget() const {
            return sceneFramebuffer ? sceneFramebuffer->getColorTarget() : nullptr;
        }

        Texture2D* getSceneBrightTarget() const {
            return sceneBrightTarget;
        }

        void render(GLuint fullscreenVao, Framebuffer* outputFramebuffer);

        void applyBloomUniforms(ShaderProgram* shader) const {
            if (shader) {
                shader->set("bloomThreshold", threshold);
                shader->set("bloomSoftKnee", softKnee);
                shader->set("bloomIntensity", intensity);
                shader->set("bloomBlurHorizontal", blurHorizontal);
                shader->set("bloomBlurVertical", blurVertical);
            }
        }
    };

}  // namespace our
