 #pragma once

#include <ecs/world.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <text/font.hpp>
#include <text/text-renderer.hpp>

namespace gameplay {

    class PlayerHUDSystem {
        our::Mesh* rectangleMesh = nullptr;
        our::ShaderProgram* progressShader = nullptr;
        our::TexturedMaterial* healthFrameMaterial = nullptr;
        our::TexturedMaterial* healthFillMaterial = nullptr;

        our::ShaderProgram* texturedShader = nullptr;
        our::TexturedMaterial* weaponMaterial = nullptr;

        our::Font testFont;

        our::UIRect weaponRect     = { {1.0f, 0.0f}, {1.0f, 0.0f}, {-20.0f, 20.0f}, {80.0f, 80.0f} };
        our::UIRect healthBarRect  = { {1.0f, 0.0f}, {1.0f, 0.0f}, {-20.0f, 30.0f}, {200.0f, 33.0f} };
        our::UIRect ammoTextRect   = { {1.0f, 0.0f}, {1.0f, 0.0f}, {-34.0f, 78.0f}, {0.0f, 0.0f} };

        float outlineThickness = 3.0f;
        float textScale = 0.3f;
        float outlineSpread = 2.0f;
        glm::vec4 ammoColor = glm::vec4(124.0f / 255.0f, 136.0f / 255.0f, 158.0f / 255.0f, 1.0f);

        std::string weaponIconPath = "assets/UI/GTASA_AK.png";
        std::string healthFramePath = "assets/UI/health_bar.png";
        std::string healthFillPath = "assets/UI/heatlh.png";
        std::string fontPath = "assets/fonts/GTASA_font/GTASA_font.json";
        std::string fontTexturePath = "assets/fonts/GTASA_font/GTASA_font.png";

    public:
        void initialize();
        void deserialize(const nlohmann::json& data);
        void render(our::World* world, our::Entity* playerEntity, glm::ivec2 windowSize, our::TextRenderer* textRenderer);
        void destroy();
    };

}  // namespace gameplay
