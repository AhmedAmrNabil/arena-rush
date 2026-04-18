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
    }

    void PlayerHUDSystem::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        paddingX = data.value("paddingX", paddingX);
        paddingY = data.value("paddingY", paddingY);
        weaponSize = data.value("weaponSize", weaponSize);
        barWidth = data.value("barWidth", barWidth);
        barHeight = data.value("barHeight", barHeight);
        outlineThickness = data.value("outlineThickness", outlineThickness);
        weaponIconPath = data.value("weaponIconPath", weaponIconPath);
    }

    void PlayerHUDSystem::render(our::World* world, our::Entity* playerEntity, glm::ivec2 windowSize) {
        HealthComponent* playerHealth = playerEntity->getComponent<HealthComponent>();
        
        float healthPercentage = playerHealth->currentHealth / playerHealth->maxHealth;
        healthPercentage = glm::clamp(healthPercentage, 0.0f, 1.0f);

        // Even though we have no view matrix here (we want the HUD fixed to the screen), VP is just a naming convention
        glm::mat4 orthoVP = glm::ortho(0.0f, (float)windowSize.x, (float)windowSize.y, 0.0f, 1.0f, -1.0f);

        // weapon icon
        glm::mat4 weaponTransform = glm::translate(glm::mat4(1.0f), glm::vec3(windowSize.x - weaponSize - paddingX, paddingY, 0.0f)) *
                                    glm::scale(glm::mat4(1.0f), glm::vec3(weaponSize, weaponSize, 1.0f));
        weaponMaterial->setup();
        weaponMaterial->shader->set("transform", orthoVP * weaponTransform);
        rectangleMesh->draw();

        // health bar
        float startX = windowSize.x - barWidth - paddingX;
        float startY = paddingY + weaponSize + 5.0f;

        // health black outline
        glm::mat4 outlineTransform = glm::translate(glm::mat4(1.0f), glm::vec3(startX - outlineThickness, startY - outlineThickness, 0.0f)) *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(barWidth + outlineThickness * 2, barHeight + outlineThickness * 2, 1.0f));
        healthBarMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        healthBarMaterial->setup();
        healthBarMaterial->shader->set("transform", orthoVP * outlineTransform);
        rectangleMesh->draw();

        // health dark read bg
        glm::mat4 bgTransform = glm::translate(glm::mat4(1.0f), glm::vec3(startX, startY, 0.0f)) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(barWidth, barHeight, 1.0f));
        healthBarMaterial->tint = glm::vec4(0.2f, 0.0f, 0.0f, 1.0f);
        healthBarMaterial->setup();
        healthBarMaterial->shader->set("transform", orthoVP * bgTransform);
        rectangleMesh->draw();

        // health bright red bg
        if (healthPercentage > 0.0f) {
            glm::mat4 healthTransform = glm::translate(glm::mat4(1.0f), glm::vec3(startX, startY, 0.0f)) *
                                        glm::scale(glm::mat4(1.0f), glm::vec3(barWidth * healthPercentage, barHeight, 1.0f));
            healthBarMaterial->tint = glm::vec4(0.85f, 0.1f, 0.1f, 1.0f);
            healthBarMaterial->setup();
            healthBarMaterial->shader->set("transform", orthoVP * healthTransform);
            rectangleMesh->draw();
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
    }

}  // namespace gameplay
