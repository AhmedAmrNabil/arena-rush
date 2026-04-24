#pragma once

#include <glad/gl.h>

#include <glm/vec2.hpp>
#include <json/json.hpp>

#include "../shader/shader.hpp"
#include "../texture/sampler.hpp"
#include "framebuffer.hpp"

namespace our {

    class BloomPostProcess {
    public:
        void initialize(glm::ivec2 windowSize, const nlohmann::json& config);
        void destroy();
        void resize(glm::ivec2 windowSize);

        Framebuffer* getSceneFramebuffer() const {
            return sceneFramebuffer;
        }

        void render(GLuint fullscreenVao, Framebuffer* outputFramebuffer);

    private:
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
        Framebuffer* brightFramebuffer = nullptr;
        Framebuffer* blurPingFramebuffer = nullptr;
        Framebuffer* blurPongFramebuffer = nullptr;

        ShaderProgram* thresholdShader = nullptr;
        ShaderProgram* blurShader = nullptr;
        ShaderProgram* combineShader = nullptr;
        Sampler* sampler = nullptr;
    };

}  // namespace our
