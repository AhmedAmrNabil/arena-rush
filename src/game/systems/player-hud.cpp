#include "player-hud.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <texture/texture-utils.hpp>

#include "../components/health.hpp"
#include "../components/player.hpp"
#include "mesh/mesh-utils.hpp"

namespace gameplay {

    void PlayerHUDSystem::initialize() {
        // a 1x1 space with local coordinates, to be scaled on render
        rectangleMesh = our::mesh_utils::generateQuad();

        progressShader = new our::ShaderProgram();
        progressShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        progressShader->attach("assets/shaders/progress.frag", GL_FRAGMENT_SHADER);
        progressShader->link();

        texturedShader = new our::ShaderProgram();
        texturedShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        texturedShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        texturedShader->link();

        healthFrameMaterial = new our::TexturedMaterial();
        healthFrameMaterial->shader = texturedShader;
        healthFrameMaterial->texture = our::texture_utils::loadImage(healthFramePath, true);
        if (healthFrameMaterial->texture) {
            healthFrameMaterial->texture->bind();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            our::Texture2D::unbind();
        }
        healthFrameMaterial->tint = glm::vec4(1.0f);
        healthFrameMaterial->pipelineState.depthTesting.enabled = false;
        healthFrameMaterial->pipelineState.faceCulling.enabled = false;
        healthFrameMaterial->pipelineState.blending.enabled = true;
        healthFrameMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        healthFrameMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        healthFillMaterial = new our::TexturedMaterial();
        healthFillMaterial->shader = progressShader;
        healthFillMaterial->texture = our::texture_utils::loadImage(healthFillPath, true);
        if (healthFillMaterial->texture) {
            healthFillMaterial->texture->bind();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            our::Texture2D::unbind();
        }
        healthFillMaterial->tint = glm::vec4(1.0f);
        healthFillMaterial->pipelineState.depthTesting.enabled = false;
        healthFillMaterial->pipelineState.faceCulling.enabled = false;
        healthFillMaterial->pipelineState.blending.enabled = true;
        healthFillMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        healthFillMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

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
        if (data.contains("ammoColor") && data["ammoColor"].is_array()) {
            auto arr = data["ammoColor"];
            ammoColor = glm::vec4(arr[0], arr[1], arr[2], arr[3]);
        }

        weaponIconPath = data.value("weaponIconPath", weaponIconPath);
        fontPath = data.value("fontPath", fontPath);
        fontTexturePath = data.value("fontTexturePath", fontTexturePath);
    }

    void PlayerHUDSystem::render(our::World* world, our::Entity* playerEntity, glm::ivec2 windowSize,
                                 our::TextRenderer* textRenderer, const EnemySpawner& spawner) {
        if (!playerEntity) return;

        HealthComponent* playerHealth = playerEntity->getComponent<HealthComponent>();
        PlayerComponent* playerComp = playerEntity->getComponent<PlayerComponent>();
        if (!playerHealth || !playerComp) return;

        float healthPercentage = playerHealth->currentHealth / playerHealth->maxHealth;
        healthPercentage = glm::clamp(healthPercentage, 0.0f, 1.0f);

        // scale all HUD elements relative to a 720p baseline
        float uiScale = windowSize.y / 720.0f;

        glm::mat4 orthoVP = glm::ortho(0.0f, (float)windowSize.x, (float)windowSize.y, 0.0f, 1.0f, -1.0f);

        // weapon icon
        our::UIRect scaledWeaponRect = weaponRect;
        scaledWeaponRect.size *= uiScale;
        scaledWeaponRect.offset *= uiScale;

        glm::vec2 weaponPos = scaledWeaponRect.getScreenPosition(windowSize);
        glm::mat4 weaponTransform =
            glm::translate(glm::mat4(1.0f), glm::vec3(weaponPos.x, weaponPos.y, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(scaledWeaponRect.size.x, scaledWeaponRect.size.y, 1.0f));

        if ((float)playerComp->currentAmmo / playerComp->magSize <= 0.2f) {
            weaponMaterial->tint = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
        } else {
            weaponMaterial->tint = glm::vec4(1.0f);
        }

        weaponMaterial->setup();
        weaponMaterial->shader->set("transform", orthoVP * weaponTransform);
        rectangleMesh->draw();

        // local copy to not accumlate the multiplies each frame
        our::UIRect scaledHealthRect = healthBarRect;
        scaledHealthRect.size *= uiScale;
        scaledHealthRect.offset *= uiScale;

        glm::vec2 barPos = scaledHealthRect.getScreenPosition(windowSize);

        // health bar frame
        glm::mat4 frameTransform =
            glm::translate(glm::mat4(1.0f), glm::vec3(barPos.x, barPos.y, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(scaledHealthRect.size.x, scaledHealthRect.size.y, 1.0f));
        healthFrameMaterial->setup();
        healthFrameMaterial->shader->set("transform", orthoVP * frameTransform);
        rectangleMesh->draw();

        if (healthPercentage > 0.0f) {
            glm::mat4 fillTransform =
                glm::translate(glm::mat4(1.0f), glm::vec3(barPos.x, barPos.y, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(scaledHealthRect.size.x, scaledHealthRect.size.y, 1.0f));
            healthFillMaterial->setup();
            healthFillMaterial->shader->set("uvScale", glm::vec2(1.0f, 1.0f));
            healthFillMaterial->shader->set("progress", healthPercentage);
            healthFillMaterial->shader->set("transform", orthoVP * fillTransform);
            rectangleMesh->draw();
        }

        // Ammo Counter
        if (textRenderer) {
            std::string ammoText =
                std::to_string(playerComp->currentAmmo) + " / " + std::to_string(playerComp->maxAmmo);

            glm::vec4 outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            our::UIRect scaledAmmoTextRect = ammoTextRect;
            scaledAmmoTextRect.size *= uiScale;
            scaledAmmoTextRect.offset *= uiScale;

            float scaledSpread = outlineSpread * uiScale;
            float sTextScale = textScale * uiScale;

            // PS2 ahh way to make outline, I can't find a solution to balance outline with scale, this is good enough
            // :) (draws the text blackened four times to create an outline)
            our::UIRect tr = scaledAmmoTextRect;
            tr.offset += glm::vec2(scaledSpread, scaledSpread);
            textRenderer->drawText(&testFont, ammoText, tr, windowSize, sTextScale, orthoVP, outlineColor);

            our::UIRect bl = scaledAmmoTextRect;
            bl.offset += glm::vec2(-scaledSpread, -scaledSpread);
            textRenderer->drawText(&testFont, ammoText, bl, windowSize, sTextScale, orthoVP, outlineColor);

            our::UIRect tl = scaledAmmoTextRect;
            tl.offset += glm::vec2(-scaledSpread, scaledSpread);
            textRenderer->drawText(&testFont, ammoText, tl, windowSize, sTextScale, orthoVP, outlineColor);

            our::UIRect br = scaledAmmoTextRect;
            br.offset += glm::vec2(scaledSpread, -scaledSpread);
            textRenderer->drawText(&testFont, ammoText, br, windowSize, sTextScale, orthoVP, outlineColor);

            textRenderer->drawText(&testFont, ammoText, scaledAmmoTextRect, windowSize, sTextScale, orthoVP, ammoColor);
        }

        // wave HUD
        glm::vec4 waveColor = glm::vec4(1.0f, 0.86f, 0.39f, 1.0f);
        if (textRenderer) {
            glm::mat4 orthoVP = glm::ortho(0.0f, (float)windowSize.x, (float)windowSize.y, 0.0f, 1.0f, -1.0f);

            glm::vec4 black(glm::vec3(0.0f), 1.0f);

            float spread = outlineSpread * uiScale;
            if (spawner.getWaveState() == EnemySpawner::WaveState::Countdown) {
                // big centered "Wave X" + countdown number
                std::string waveText = "Wave " + std::to_string(spawner.getWaveDisplayNumber());
                float bigScale = waveBigTextScale * uiScale;
                our::UIRect bigRect;
                bigRect.anchor = {0.5f, 0.5f};
                bigRect.pivot = {0.5f, 0.5f};
                bigRect.offset = {0.0f, -100.0f * uiScale};
                bigRect.size = {0.0f, 0.0f};
                // outline
                for (auto off : std::vector<glm::vec2>{
                         {spread, spread}, {-spread, -spread}, {-spread, spread}, {spread, -spread}}) {
                    our::UIRect o = bigRect;
                    o.offset += off;
                    textRenderer->drawText(&testFont, waveText, o, windowSize, bigScale, orthoVP, black);
                }
                textRenderer->drawText(&testFont, waveText, bigRect, windowSize, bigScale, orthoVP, waveColor);
                // countdown
                int ct = std::max(1, (int)std::ceil(spawner.getCountdownTimer()));
                std::string cdText = std::to_string(ct);
                our::UIRect cdRect = bigRect;
                cdRect.offset.y += 100.0f * uiScale;
                for (auto off : std::vector<glm::vec2>{
                         {spread, spread}, {-spread, -spread}, {-spread, spread}, {spread, -spread}}) {
                    our::UIRect o = cdRect;
                    o.offset += off;
                    textRenderer->drawText(&testFont, cdText, o, windowSize, bigScale, orthoVP, black);
                }
                textRenderer->drawText(&testFont, cdText, cdRect, windowSize, bigScale, orthoVP, waveWhite);
            } else {
                // small top-center "Wave X  |  Enemies: N"
                std::string info = "Wave " + std::to_string(spawner.getWaveDisplayNumber()) +
                                   "   Enemies: " + std::to_string(spawner.getEnemiesRemaining(world));
                float smallScale = waveTextScale * uiScale;
                our::UIRect infoRect;
                infoRect.anchor = {0.5f, 0.0f};
                infoRect.pivot = {0.5f, 0.0f};
                infoRect.offset = {0.0f, 20.0f * uiScale};
                infoRect.size = {0.0f, 0.0f};

                // clang-format off
                std::vector<glm::vec2> offsets = {{spread, spread}, {-spread, -spread}, {-spread, spread}, {spread, -spread}};

                textRenderer->drawTextWithOutline(&testFont, info, infoRect, windowSize, smallScale, orthoVP, waveColor, black, spread, offsets);
                // clang-format on
            }
        }
    }

    void PlayerHUDSystem::destroy() {
        if (progressShader) delete progressShader;
        if (rectangleMesh) delete rectangleMesh;
        if (texturedShader) delete texturedShader;
        if (healthFrameMaterial) {
            if (healthFrameMaterial->texture) delete healthFrameMaterial->texture;
            delete healthFrameMaterial;
        }
        if (healthFillMaterial) {
            if (healthFillMaterial->texture) delete healthFillMaterial->texture;
            delete healthFillMaterial;
        }

        if (weaponMaterial) {
            if (weaponMaterial->texture) delete weaponMaterial->texture;
            delete weaponMaterial;
        }

        testFont.destroy();
    }

}  // namespace gameplay
