#pragma once

#include <application.hpp>
#include <glm/common.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <texture/texture-utils.hpp>
#include <texture/texture2d.hpp>

#include "menu-layout.hpp"

class LoadingPlayState : public our::State {
    our::TexturedMaterial* loadingMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    float time = 0.0f;
    bool queuedTransition = false;

public:
    void onInitialize() override {
        loadingMaterial = new our::TexturedMaterial();
        loadingMaterial->shader = new our::ShaderProgram();
        loadingMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        loadingMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        loadingMaterial->shader->link();
        loadingMaterial->texture = our::texture_utils::loadImage("assets/textures/menus/loading.png");
        loadingMaterial->tint = glm::vec4(1.0f);

        rectangle = new our::Mesh(
            {
                {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            },
            {
                0,
                1,
                2,
                2,
                3,
                0,
            });

        time = 0.0f;
        queuedTransition = false;
    }

    void onDraw(double deltaTime) override {
        auto& keyboard = getApp()->getKeyboard();
        if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            getApp()->changeState("menu");
            return;
        }

        glm::ivec2 size = getApp()->getFrameBufferSize();
        menu_ui::PixelRect menuRect = menu_ui::getMenuRect(size);

        glViewport(0, 0, size.x, size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 VP = glm::ortho(0.0f, static_cast<float>(size.x), static_cast<float>(size.y), 0.0f, 1.0f, -1.0f);
        glm::mat4 M = menuRect.getLocalToWorld();

        time += static_cast<float>(deltaTime);
        float pulse = 0.92f + 0.08f * glm::sin(time * 4.0f);
        loadingMaterial->tint = glm::vec4(pulse, pulse, pulse, 1.0f);
        loadingMaterial->setup();
        loadingMaterial->shader->set("transform", VP * M);
        rectangle->draw();

        if (!queuedTransition) {
            queuedTransition = true;
            getApp()->changeState("play");
        }
    }

    void onDestroy() override {
        delete rectangle;
        delete loadingMaterial->texture;
        delete loadingMaterial->shader;
        delete loadingMaterial;
    }
};
