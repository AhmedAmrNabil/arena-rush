#include "bloom.hpp"

namespace our {

    void BloomPostProcess::initialize(glm::ivec2 windowSize, const nlohmann::json& config) {
        this->windowSize = windowSize;

        threshold = config.value("threshold", threshold);
        softKnee = config.value("softKnee", softKnee);
        radius = config.value("radius", radius);
        intensity = config.value("intensity", intensity);

        if (config.contains("radius")) {
            blurHorizontal = radius;
            blurVertical = radius;
        } else if (config.contains("blur") && config["blur"].is_object()) {
            const auto& blurConfig = config["blur"];
            blurHorizontal = blurConfig.value("horizontal", blurHorizontal);
            blurVertical = blurConfig.value("vertical", blurVertical);
        } else {
            blurHorizontal = config.value("blurHorizontal", blurHorizontal);
            blurVertical = config.value("blurVertical", blurVertical);
        }

        sceneFramebuffer = new Framebuffer();
        sceneFramebuffer->resize(windowSize, true);

        sceneBrightTarget = new Texture2D();
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer->frameBuffer);
        sceneBrightTarget->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, sceneBrightTarget->getOpenGLName(),
                               0);
        Texture2D::unbind();
        Framebuffer::unbind();

        blurPingFramebuffer = new Framebuffer();
        blurPongFramebuffer = new Framebuffer();
        blurPingFramebuffer->resize(windowSize, true);
        blurPongFramebuffer->resize(windowSize, true);

        sampler = new Sampler();
        sampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        sampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        sampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        sampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        blurShader = new ShaderProgram();
        blurShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
        blurShader->attach("assets/shaders/postprocess/bloom-blur.frag", GL_FRAGMENT_SHADER);
        blurShader->link();

        combineShader = new ShaderProgram();
        combineShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
        combineShader->attach("assets/shaders/postprocess/bloom-combine.frag", GL_FRAGMENT_SHADER);
        combineShader->link();
    }

    void BloomPostProcess::destroy() {
        delete sceneFramebuffer;
        delete sceneBrightTarget;
        delete blurPingFramebuffer;
        delete blurPongFramebuffer;
        delete blurShader;
        delete combineShader;
        delete sampler;

        sceneFramebuffer = nullptr;
        sceneBrightTarget = nullptr;
        blurPingFramebuffer = nullptr;
        blurPongFramebuffer = nullptr;
        blurShader = nullptr;
        combineShader = nullptr;
        sampler = nullptr;
    }

    void BloomPostProcess::resize(glm::ivec2 windowSize) {
        this->windowSize = windowSize;
        if (!sceneFramebuffer) return;
        sceneFramebuffer->resize(windowSize, true);
        if (!sceneBrightTarget) {
            sceneBrightTarget = new Texture2D();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer->frameBuffer);
        sceneBrightTarget->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, sceneBrightTarget->getOpenGLName(),
                               0);
        Texture2D::unbind();
        Framebuffer::unbind();
        blurPingFramebuffer->resize(windowSize, true);
        blurPongFramebuffer->resize(windowSize, true);
    }

    void BloomPostProcess::bindSceneFramebuffer() const {
        if (!sceneFramebuffer) return;
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer->frameBuffer);
        GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);
    }

    void BloomPostProcess::setupFullscreenState() const {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    void BloomPostProcess::drawFullscreen(GLuint fullscreenVao) const {
        glBindVertexArray(fullscreenVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    void BloomPostProcess::render(GLuint fullscreenVao, Framebuffer* outputFramebuffer) {
        if (!sceneFramebuffer || fullscreenVao == 0) return;

        setupFullscreenState();

        // first pass: blur horizontally
        blurPingFramebuffer->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        blurShader->use();
        glActiveTexture(GL_TEXTURE0);
        if (sceneBrightTarget) {
            sceneBrightTarget->bind();
        }
        sampler->bind(0);
        blurShader->set("tex", 0);
        blurShader->set("direction", glm::vec2(1.0f, 0.0f));
        blurShader->set("radius", blurHorizontal);
        drawFullscreen(fullscreenVao);

        // second pass: blur vertically
        blurPongFramebuffer->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        blurShader->use();
        glActiveTexture(GL_TEXTURE0);
        blurPingFramebuffer->getColorTarget()->bind();
        sampler->bind(0);
        blurShader->set("tex", 0);
        blurShader->set("direction", glm::vec2(0.0f, 1.0f));
        blurShader->set("radius", blurVertical);
        drawFullscreen(fullscreenVao);

        // third pass: combine bloom with the original scene
        if (outputFramebuffer) {
            outputFramebuffer->bind();
        } else {
            Framebuffer::unbind();
        }
        glClear(GL_COLOR_BUFFER_BIT);

        combineShader->use();
        glActiveTexture(GL_TEXTURE0);
        sceneFramebuffer->getColorTarget()->bind();
        sampler->bind(0);
        combineShader->set("sceneTex", 0);

        glActiveTexture(GL_TEXTURE1);
        blurPongFramebuffer->getColorTarget()->bind();
        sampler->bind(1);
        combineShader->set("bloomTex", 1);
        combineShader->set("intensity", intensity);
        drawFullscreen(fullscreenVao);
    }

}  // namespace our
