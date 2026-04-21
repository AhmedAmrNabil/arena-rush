#pragma once

#include <application.hpp>
#include <deserialize-utils.hpp>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <systems/ui-renderer.hpp>

namespace gameplay {
    class CrosshairRenderer {
        struct Config {
            float armLengthFrac = 0.012f;      // arm length as fraction of screen height
            float armThicknessFrac = 0.0025f;  // arm thickness as fraction of screen height
            float gapFrac = 0.006f;            // gap from center as fraction of screen height
            glm::vec4 shadowColor = {0.02f, 0.03f, 0.05f, 0.92f};
            glm::vec4 fillColor = {0.95f, 0.98f, 1.0f, 0.94f};
        } config;

        void drawSegment(const our::UIRenderer& ui, const glm::mat4& projection, const glm::vec2& position,
                         const glm::vec2& segmentSize, float shadowPad) const {
            ui.drawRect(projection, position - glm::vec2(shadowPad), segmentSize + glm::vec2(shadowPad * 2.0f),
                        config.shadowColor);
            ui.drawRect(projection, position, segmentSize, config.fillColor);
        }

    public:
        void deserialize(const nlohmann::json& sceneConfig) {
            if (!(sceneConfig.contains("game") && sceneConfig["game"].contains("crosshair"))) return;

            const auto& crosshairConfig = sceneConfig["game"]["crosshair"];
            if (!crosshairConfig.is_object()) return;

            config = {};
            config.armLengthFrac = glm::max(0.0f, crosshairConfig.value("armLengthFrac", config.armLengthFrac));
            config.armThicknessFrac =
                glm::max(0.0f, crosshairConfig.value("armThicknessFrac", config.armThicknessFrac));
            config.gapFrac = glm::max(0.0f, crosshairConfig.value("gapFrac", config.gapFrac));

            if (crosshairConfig.contains("shadowColor") && crosshairConfig["shadowColor"].is_array())
                config.shadowColor = crosshairConfig["shadowColor"].get<glm::vec4>();

            if (crosshairConfig.contains("fillColor") && crosshairConfig["fillColor"].is_array())
                config.fillColor = crosshairConfig["fillColor"].get<glm::vec4>();
        }

        void render(our::Application* app, const our::UIRenderer& ui, float aimBlend = 0.0f) const {
            if (!app) return;

            glm::ivec2 size = app->getFrameBufferSize();
            if (size.x <= 0 || size.y <= 0) return;

            glm::mat4 projection = ui.overlayProjection(size);
            glm::vec2 center = {size.x * 0.5f, size.y * 0.5f};

            float t = glm::clamp(aimBlend, 0.0f, 1.0f);
            float h = static_cast<float>(size.y);

            float shadowPad = glm::max(1.0f, h * 0.0013f);

            // base sizes when not aiming
            float armBase = h * config.armLengthFrac;
            float thick = glm::max(1.0f, h * config.armThicknessFrac);
            float gapBase = h * config.gapFrac;

            // make it smaller when aiming
            float arm = glm::mix(armBase, armBase * 0.45f, t);
            float gap = glm::mix(gapBase, glm::max(1.0f, h * 0.001f), t);
            float dot = glm::max(2.0f, thick);

            drawSegment(ui, projection, {center.x - thick * 0.5f, center.y - gap - arm}, {thick, arm},
                        shadowPad);                                                                           // top
            drawSegment(ui, projection, {center.x - thick * 0.5f, center.y + gap}, {thick, arm}, shadowPad);  // bottom
            drawSegment(ui, projection, {center.x - gap - arm, center.y - thick * 0.5f}, {arm, thick},
                        shadowPad);                                                                           // left
            drawSegment(ui, projection, {center.x + gap, center.y - thick * 0.5f}, {arm, thick}, shadowPad);  // right
            drawSegment(ui, projection, center - glm::vec2(dot * 0.5f), {dot, dot}, shadowPad);               // dot
        }
    };

}  // namespace gameplay
