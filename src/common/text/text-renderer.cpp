#include "text-renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace our {

    void TextRenderer::initialize() {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TextVertex), (void*)offsetof(TextVertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, tex_coord));
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        shader = new our::ShaderProgram();
        shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        shader->link();

        material = new our::TexturedMaterial();
        material->shader = shader;
        material->pipelineState.depthTesting.enabled = false;
        material->pipelineState.faceCulling.enabled = false;
        material->pipelineState.blending.enabled = true;
        material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        material->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;
    }

    void TextRenderer::drawText(Font* font, const std::string& text, float startX, float startY, float scale,
                                const glm::mat4& VP, glm::vec4 color) {
        if (!font || !font->getTexture() || text.empty()) return;

        std::vector<TextVertex> vertices;
        std::vector<unsigned int> indices;

        float cursorX = startX;
        float cursorY = startY;
        float texWidth = (float)font->getTexture()->getWidth();
        float texHeight = (float)font->getTexture()->getHeight();

        if (texWidth == 0 || texHeight == 0) return;

        unsigned int vertexOffset = 0;

        for (char c : text) {
            const CharacterInfo* info = font->getCharacter(c);
            if (!info) {
                // If it's a space, advance the cursor and skip drawing
                if (c == ' ') cursorX += font->getFontSize() * 0.4f;
                continue;
            }

            // Calculate the 4 corners of the letter on the Screen with scaling applied
            float x0 = cursorX + info->xoffset * scale;
            float y0 = cursorY + info->yoffset * scale;
            float x1 = x0 + info->width * scale;
            float y1 = y0 + info->height * scale;

            // Calculate the Texture Coordinates from the BMFont Atlas
            float u0 = (float)info->x / texWidth;
            float v0 = 1.0f - ((float)info->y / texHeight);  // Top (inverted for OpenGL)
            float u1 = (float)(info->x + info->width) / texWidth;
            float v1 = 1.0f - ((float)(info->y + info->height) / texHeight);  // Bottom

            // Generate the vertices for this letter
            vertices.push_back({{x0, y0, 0}, {255, 255, 255, 255}, {u0, v0}});
            vertices.push_back({{x1, y0, 0}, {255, 255, 255, 255}, {u1, v0}});
            vertices.push_back({{x1, y1, 0}, {255, 255, 255, 255}, {u1, v1}});
            vertices.push_back({{x0, y1, 0}, {255, 255, 255, 255}, {u0, v1}});

            // Two triangles per letter block
            indices.push_back(vertexOffset + 0);
            indices.push_back(vertexOffset + 1);
            indices.push_back(vertexOffset + 2);
            indices.push_back(vertexOffset + 2);
            indices.push_back(vertexOffset + 3);
            indices.push_back(vertexOffset + 0);

            vertexOffset += 4;
            cursorX += info->xadvance * scale;  // Push the cursor forward for the next letter
        }

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TextVertex), vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
        elementCount = (GLsizei)indices.size();

        material->texture = font->getTexture();
        material->tint = color;
        material->setup();

        material->shader->set("transform", VP);

        glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void TextRenderer::drawText(Font* font, const std::string& text, UIRect rect, glm::vec2 windowSize, float scale,
                                const glm::mat4& VP, glm::vec4 color) {
        if (!font || text.empty()) return;

        // compute the width of the bounding box to put it into UIRect.size.x
        float trueTextWidth = 0.0f;
        for (size_t i = 0; i < text.length(); i++) {
            char c = text[i];
            const CharacterInfo* info = font->getCharacter(c);
            if (info) {
                if (i == text.length() - 1) {
                    trueTextWidth += (info->xoffset + info->width) * scale;
                } else {
                    trueTextWidth += info->xadvance * scale;
                }
            }
        }

        rect.size.x = trueTextWidth;

        glm::vec2 pos = rect.getScreenPosition(windowSize);
        drawText(font, text, pos.x, pos.y, scale, VP, color);
    }

    void TextRenderer::drawTextWithOutline(Font* font, const std::string& text, UIRect rect, glm::vec2 windowSize,
                                           float scale, const glm::mat4& VP, glm::vec4 textColor,
                                           glm::vec4 outlineColor, float outlineSpread,
                                           const std::vector<glm::vec2> offsets) {
        for (const auto& off : offsets) {
            UIRect o = rect;
            o.offset += off * outlineSpread;
            drawText(font, text, o, windowSize, scale, VP, outlineColor);
        }

        drawText(font, text, rect, windowSize, scale, VP, textColor);
    }

    void TextRenderer::destroy() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (EBO) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
        if (shader) {
            delete shader;
            shader = nullptr;
        }
        if (material) {
            delete material;
            material = nullptr;
        }
    }

}  // namespace our
