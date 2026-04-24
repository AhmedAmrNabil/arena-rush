#pragma once
#include <string>
#include "font.hpp"
#include <mesh/mesh.hpp>
#include <material/material.hpp>

#include "font.hpp"

namespace our {

    struct TextVertex {
        glm::vec3 position;
        glm::u8vec4 color;
        glm::vec2 tex_coord;
    };

    struct UIRect {
        glm::vec2 anchor = {0.0f, 0.0f}; // Normalized screen attachment point (0 to 1)
        // pivot should always be EQUAL to anchor for static elements, for dynamic elements like if we need some 'achievement/powerup' text to slide in from the top center for example, we set anchor to (0.5, 0), top center of the screen, and pivot to (0.5, 1), bottom center of the rectangle, so the rectangle is completely hidden above the screen, we then change offset gradually to bring it into view 
        glm::vec2 pivot = {0.0f, 0.0f};  // Normalized element origin (0 to 1).
        glm::vec2 offset = {0.0f, 0.0f}; // Pixel offset (X, Y)
        glm::vec2 size = {0.0f, 0.0f};   // Width and Height in pixels

        glm::vec2 getScreenPosition(glm::vec2 windowSize) const {
            glm::vec2 screenAnchorPos = windowSize * anchor;
            glm::vec2 pivotOffset = size * pivot;
            return screenAnchorPos - pivotOffset + offset;
        }
    };

    class TextRenderer {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;
        GLsizei elementCount = 0;

        our::ShaderProgram* shader = nullptr;
        our::TexturedMaterial* material = nullptr;

        void drawText(Font* font, const std::string& text, float startX, float startY, float scale, const glm::mat4& orthoVP, glm::vec4 color = glm::vec4(1.0f));

    public:
        void initialize();

        // an abstraction over private drawText using the UIRect
        void drawText(Font* font, const std::string& text, UIRect rect, glm::vec2 windowSize, float scale, const glm::mat4& orthoVP, glm::vec4 color = glm::vec4(1.0f));

        void destroy();
    };

}
