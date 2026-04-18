#pragma once

#include <ecs/world.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>

namespace gameplay {

    class PlayerHUDSystem {
        our::Mesh* rectangleMesh = nullptr;
        our::ShaderProgram* tintShader = nullptr;
        our::TintedMaterial* healthBarMaterial = nullptr;

        our::ShaderProgram* texturedShader = nullptr;
        our::TexturedMaterial* weaponMaterial = nullptr;

        float paddingX = 20.0f;
        float paddingY = 20.0f;
        float weaponSize = 80.0f;
        float barWidth = 160.0f;
        float barHeight = 9.0f;
        float outlineThickness = 3.0f;
        std::string weaponIconPath = "assets/UI/GTASA_AK.png";

    public:
        void initialize();
        void deserialize(const nlohmann::json& data);
        void render(our::World* world, our::Entity* playerEntity, glm::ivec2 windowSize);
        void destroy();
    };

}  // namespace gameplay
