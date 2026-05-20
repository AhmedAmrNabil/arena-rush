#pragma once

#include <algorithm>
#include <application.hpp>
#include <array>
#include <audio/audio-buffer.hpp>
#include <audio/audio-utils.hpp>
#include <glm/common.hpp>
#include <material/material.hpp>
#include <mesh/mesh-utils.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <string>
#include <texture/texture-utils.hpp>
#include <texture/texture2d.hpp>

#include "menu-layout.hpp"

class Menustate : public our::State {
    our::TexturedMaterial* menuMaterial = nullptr;
    our::TintedMaterial* highlightMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    float time = 0.0f;
    std::array<menu_ui::Button, 2> buttons;
    our::AudioBuffer* menuMusic = nullptr;
    our::AudioBuffer* menuSelectSound = nullptr;
    int hoveredButton = -1;

public:
    void onInitialize(GLFWwindow*) override {
        menuMaterial = new our::TexturedMaterial();
        menuMaterial->shader = new our::ShaderProgram();
        menuMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        menuMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        menuMaterial->shader->link();
        menuMaterial->texture = our::texture_utils::loadImage("assets/textures/menus/main-menu.png");
        menuMaterial->tint = glm::vec4(0.0f);

        highlightMaterial = new our::TintedMaterial();
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        highlightMaterial->tint = glm::vec4(1.0f);
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_SUBTRACT;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_ONE;
        highlightMaterial->pipelineState.blending.destinationFactor = GL_ONE;

        rectangle = our::mesh_utils::generateQuad();

        time = 0.0f;
        hoveredButton = -1;

        buttons[0].normalizedPosition = {0.34375f, 0.6875f};
        buttons[0].normalizedSize = {0.31328125f, 0.10078125f};
        buttons[0].action = [this]() { getApp()->changeState("loading-play"); };

        buttons[1].normalizedPosition = {0.34375f, 0.8125f};
        buttons[1].normalizedSize = {0.31328125f, 0.10078125f};
        buttons[1].action = [this]() { getApp()->close(); };

        menuMusic = our::audio_utils::loadAudio("assets/sounds/menu-music.ogg");
        menuSelectSound = our::audio_utils::loadAudio("assets/sounds/menu-select.ogg");
        getApp()->getAudioSystem().playSound2D(menuMusic, 0.5f, 1.0f, true);
    }

    void onDraw(double deltaTime) override {
        auto& keyboard = getApp()->getKeyboard();
        auto& joystick = getApp()->getJoystick();
        if (keyboard.justPressed(GLFW_KEY_SPACE) || joystick.justPressed(GLFW_GAMEPAD_BUTTON_A))
            getApp()->changeState("loading-play");
        else if (keyboard.justPressed(GLFW_KEY_ESCAPE) || joystick.justPressed(GLFW_GAMEPAD_BUTTON_B))
            getApp()->close();

        glm::ivec2 size = getApp()->getFrameBufferSize();
        menu_ui::PixelRect menuRect = menu_ui::getMenuRect(size);

        auto& mouse = getApp()->getMouse();
        glm::vec2 mousePosition = menu_ui::getMousePositionInTarget(getApp(), size);
        if (mouse.justPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            for (auto& button : buttons) {
                if (button.getPixelRect(menuRect).isInside(mousePosition)) {
                    button.action();
                    break;
                }
            }
        }

        glViewport(0, 0, size.x, size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = menuRect.getLocalToWorld();

        time += static_cast<float>(deltaTime);
        menuMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 2.0f, time));
        menuMaterial->setup();
        menuMaterial->shader->set("transform", VP * M);
        rectangle->draw();

        int currentlyHovered = -1;
        for (int i = 0; i < static_cast<int>(buttons.size()); i++) {
            if (buttons[i].getPixelRect(menuRect).isInside(mousePosition)) {
                currentlyHovered = i;
                break;
            }
        }

        if (currentlyHovered != -1 && currentlyHovered != hoveredButton)
            getApp()->getAudioSystem().playSound2D(menuSelectSound, 1.0f, 1.0f, false);

        hoveredButton = currentlyHovered;

        if (currentlyHovered != -1) {
            menu_ui::PixelRect hoveredRect = buttons[currentlyHovered].getPixelRect(menuRect);
            highlightMaterial->setup();
            highlightMaterial->shader->set("transform", VP * hoveredRect.getLocalToWorld());
            rectangle->draw();
        }
    }

    void onDestroy() override {
        delete rectangle;
        delete menuMaterial->texture;
        delete menuMaterial->shader;
        delete menuMaterial;
        delete highlightMaterial->shader;
        delete highlightMaterial;
        getApp()->getAudioSystem().stopAll();
        delete menuMusic;
        delete menuSelectSound;
    }
};
