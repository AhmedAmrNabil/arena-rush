#include "player-hud.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <texture/texture-utils.hpp>

#include "../components/health.hpp"
#include "../components/player.hpp"

namespace gameplay {

    void PlayerHUDSystem::initialize() {
        // a 1x1 space with local coordinates, to be scaled on render
        rectangleMesh = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }, {
            0, 1, 2, 2, 3, 0,
        });

        tintShader = new our::ShaderProgram();
        tintShader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        tintShader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        tintShader->link();

        healthBarMaterial = new our::TintedMaterial();
        healthBarMaterial->shader = tintShader;
        healthBarMaterial->pipelineState.depthTesting.enabled = false;
        healthBarMaterial->pipelineState.faceCulling.enabled = false;

        texturedShader = new our::ShaderProgram();
        texturedShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        texturedShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        texturedShader->link();

        weaponMaterial = new our::TexturedMaterial();
        weaponMaterial->shader = texturedShader;
        weaponMaterial->texture = our::texture_utils::loadImage(weaponIconPath, false);
        weaponMaterial->tint = glm::vec4(1.0f);
        weaponMaterial->pipelineState.depthTesting.enabled = false;
        weaponMaterial->pipelineState.faceCulling.enabled = false;
        // weapon png has transparent parts
        weaponMaterial->pipelineState.blending.enabled = true;
        weaponMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        weaponMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        testFont.load(fontPath, fontTexturePath);
    }

    void PlayerHUDSystem::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        auto parseRect = [](const nlohmann::json& j, our::UIRect& rect) {
            if (j.contains("anchor")) rect.anchor = {j["anchor"][0], j["anchor"][1]};
            if (j.contains("pivot")) rect.pivot = {j["pivot"][0], j["pivot"][1]};
            if (j.contains("offset")) rect.offset = {j["offset"][0], j["offset"][1]};
            if (j.contains("size")) rect.size = {j["size"][0], j["size"][1]};
        };

        if (data.contains("weaponRect")) parseRect(data["weaponRect"], weaponRect);
        if (data.contains("healthBarRect")) parseRect(data["healthBarRect"], healthBarRect);
        if (data.contains("ammoTextRect")) parseRect(data["ammoTextRect"], ammoTextRect);

        outlineThickness = data.value("outlineThickness", outlineThickness);
        textScale = data.value("textScale", textScale);
        outlineSpread = data.value("outlineSpread", outlineSpread);
        if(data.contains("ammoColor") && data["ammoColor"].is_array()) {
            auto arr = data["ammoColor"];
            ammoColor = glm::vec4(arr[0], arr[1], arr[2], arr[3]);
        }
        
        weaponIconPath = data.value("weaponIconPath", weaponIconPath);
        fontPath = data.value("fontPath", fontPath);
        fontTexturePath = data.value("fontTexturePath", fontTexturePath);
    }

    void PlayerHUDSystem::render(our::World* world, our::Entity* playerEntity, glm::ivec2 windowSize, our::TextRenderer* textRenderer) {
        if (!playerEntity) return;

        HealthComponent* playerHealth = playerEntity->getComponent<HealthComponent>();
        PlayerComponent* playerComp = playerEntity->getComponent<PlayerComponent>();
        if (!playerHealth || !playerComp) return;
        
        float healthPercentage = playerHealth->currentHealth / playerHealth->maxHealth;
        healthPercentage = glm::clamp(healthPercentage, 0.0f, 1.0f);

        // Even though we have no view matrix here (we want the HUD fixed to the screen), VP is just a naming convention
        glm::mat4 orthoVP = glm::ortho(0.0f, (float)windowSize.x, (float)windowSize.y, 0.0f, 1.0f, -1.0f);

        // weapon icon
        glm::vec2 weaponPos = weaponRect.getScreenPosition(windowSize);
        glm::mat4 weaponTransform = glm::translate(glm::mat4(1.0f), glm::vec3(weaponPos.x, weaponPos.y, 0.0f)) *
                                    glm::scale(glm::mat4(1.0f), glm::vec3(weaponRect.size.x, weaponRect.size.y, 1.0f));
        weaponMaterial->setup();
        weaponMaterial->shader->set("transform", orthoVP * weaponTransform);
        rectangleMesh->draw();

        glm::vec2 barPos = healthBarRect.getScreenPosition(windowSize);

        // health black outline
        glm::mat4 outlineTransform = glm::translate(glm::mat4(1.0f), glm::vec3(barPos.x - outlineThickness, barPos.y - outlineThickness, 0.0f)) *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(healthBarRect.size.x + outlineThickness * 2, healthBarRect.size.y + outlineThickness * 2, 1.0f));
        healthBarMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        healthBarMaterial->setup();
        healthBarMaterial->shader->set("transform", orthoVP * outlineTransform);
        rectangleMesh->draw();

        // health dark read bg
        glm::mat4 bgTransform = glm::translate(glm::mat4(1.0f), glm::vec3(barPos.x, barPos.y, 0.0f)) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(healthBarRect.size.x, healthBarRect.size.y, 1.0f));
        healthBarMaterial->tint = glm::vec4(0.2f, 0.0f, 0.0f, 1.0f);
        healthBarMaterial->setup();
        healthBarMaterial->shader->set("transform", orthoVP * bgTransform);
        rectangleMesh->draw();

        // health bright red bg
        if (healthPercentage > 0.0f) {
            glm::mat4 healthTransform = glm::translate(glm::mat4(1.0f), glm::vec3(barPos.x, barPos.y, 0.0f)) *
                                        glm::scale(glm::mat4(1.0f), glm::vec3(healthBarRect.size.x * healthPercentage, healthBarRect.size.y, 1.0f));
            healthBarMaterial->tint = glm::vec4(0.85f, 0.1f, 0.1f, 1.0f);
            healthBarMaterial->setup();
            healthBarMaterial->shader->set("transform", orthoVP * healthTransform);
            rectangleMesh->draw();
        }

        // Ammo Counter
        if (textRenderer) {
            std::string ammoText = std::to_string(playerComp->currentAmmo) + "-" + std::to_string(playerComp->maxAmmo);
            
            glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            
            // PS2 ahh way to make outline, I can't find a solution to balance outline with scale, this is good enough :)
            // (draws the text blackened four times to create an outline)
            our::UIRect tr = ammoTextRect; tr.offset += glm::vec2(outlineSpread, outlineSpread);
            textRenderer->drawText(&testFont, ammoText, tr, windowSize, textScale, orthoVP, outlineColor);
            
            our::UIRect bl = ammoTextRect; bl.offset += glm::vec2(-outlineSpread, -outlineSpread);
            textRenderer->drawText(&testFont, ammoText, bl, windowSize, textScale, orthoVP, outlineColor);
            
            our::UIRect tl = ammoTextRect; tl.offset += glm::vec2(-outlineSpread, outlineSpread);
            textRenderer->drawText(&testFont, ammoText, tl, windowSize, textScale, orthoVP, outlineColor);
            
            our::UIRect br = ammoTextRect; br.offset += glm::vec2(outlineSpread, -outlineSpread);
            textRenderer->drawText(&testFont, ammoText, br, windowSize, textScale, orthoVP, outlineColor);

            textRenderer->drawText(&testFont, ammoText, ammoTextRect, windowSize, textScale, orthoVP, ammoColor);
        }
    }

    void PlayerHUDSystem::destroy() {
        if (rectangleMesh) delete rectangleMesh;
        if (tintShader) delete tintShader;
        if (healthBarMaterial) delete healthBarMaterial;

        if (texturedShader) delete texturedShader;
        if (weaponMaterial) {
            if (weaponMaterial->texture) delete weaponMaterial->texture;
            delete weaponMaterial;
        }

        testFont.destroy();
    }

}  // namespace gameplay
