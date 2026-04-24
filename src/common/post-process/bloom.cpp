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
        brightFramebuffer = new Framebuffer();
        blurPingFramebuffer = new Framebuffer();
        blurPongFramebuffer = new Framebuffer();

        sceneFramebuffer->resize(windowSize, true);
        brightFramebuffer->resize(windowSize, true);
        blurPingFramebuffer->resize(windowSize, true);
        blurPongFramebuffer->resize(windowSize, true);

        sampler = new Sampler();
        sampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        sampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        sampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        sampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        thresholdShader = new ShaderProgram();
        thresholdShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
        thresholdShader->attach("assets/shaders/postprocess/bloom-threshold.frag", GL_FRAGMENT_SHADER);
        thresholdShader->link();

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
        delete brightFramebuffer;
        delete blurPingFramebuffer;
        delete blurPongFramebuffer;
        delete thresholdShader;
        delete blurShader;
        delete combineShader;
        delete sampler;

        sceneFramebuffer = nullptr;
        brightFramebuffer = nullptr;
        blurPingFramebuffer = nullptr;
        blurPongFramebuffer = nullptr;
        thresholdShader = nullptr;
        blurShader = nullptr;
        combineShader = nullptr;
        sampler = nullptr;
    }

    void BloomPostProcess::resize(glm::ivec2 windowSize) {
        this->windowSize = windowSize;
        if (!sceneFramebuffer) return;
        sceneFramebuffer->resize(windowSize, true);
        brightFramebuffer->resize(windowSize, true);
        blurPingFramebuffer->resize(windowSize, true);
        blurPongFramebuffer->resize(windowSize, true);
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

        brightFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        thresholdShader->use();
        glActiveTexture(GL_TEXTURE0);
        sceneFramebuffer->getColorTarget()->bind();
        sampler->bind(0);
        thresholdShader->set("tex", 0);
        thresholdShader->set("threshold", threshold);
        thresholdShader->set("softKnee", softKnee);
        drawFullscreen(fullscreenVao);

        blurPingFramebuffer->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        blurShader->use();
        glActiveTexture(GL_TEXTURE0);
        brightFramebuffer->getColorTarget()->bind();
        sampler->bind(0);
        blurShader->set("tex", 0);
        blurShader->set("direction", glm::vec2(1.0f, 0.0f));
        blurShader->set("radius", blurHorizontal);
        drawFullscreen(fullscreenVao);

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
