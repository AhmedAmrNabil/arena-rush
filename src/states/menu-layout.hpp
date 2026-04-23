#pragma once

#include <application.hpp>
#include <functional>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace menu_ui {

    struct PixelRect {
        glm::vec2 position, size;

        bool isInside(const glm::vec2& point) const {
            return position.x <= point.x && position.y <= point.y && point.x <= position.x + size.x &&
                   point.y <= position.y + size.y;
        }

        glm::mat4 getLocalToWorld() const {
            return glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) *
                   glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
        }
    };

    struct Button {
        glm::vec2 normalizedPosition, normalizedSize;
        std::function<void()> action;

        PixelRect getPixelRect(const PixelRect& menuRect) const {
            return {
                menuRect.position + normalizedPosition * menuRect.size,
                normalizedSize * menuRect.size,
            };
        }
    };

    inline PixelRect getMenuRect(glm::ivec2 targetSize) {
        glm::vec2 viewportSize(targetSize);
        constexpr float menuAspectRatio = 16.0f / 9.0f;

        glm::vec2 contentSize = viewportSize;
        if (viewportSize.x / viewportSize.y > menuAspectRatio)
            contentSize.x = viewportSize.y * menuAspectRatio;
        else
            contentSize.y = viewportSize.x / menuAspectRatio;

        return {(viewportSize - contentSize) * 0.5f, contentSize};
    }

    inline glm::vec2 getMousePositionInTarget(our::Application* app, glm::ivec2 targetSize) {
        glm::vec2 mousePosition = app->getMouse().getMousePosition();
        glm::ivec2 windowSize = app->getWindowSize();
        if (windowSize.x <= 0 || windowSize.y <= 0) return mousePosition;

        return {
            mousePosition.x * targetSize.x / static_cast<float>(windowSize.x),
            mousePosition.y * targetSize.y / static_cast<float>(windowSize.y),
        };
    }

}  // namespace menu_ui
