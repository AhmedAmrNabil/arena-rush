#pragma once

#include <application.hpp>
#include <asset-loader.hpp>
#include <atomic>
#include <glm/common.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <model/model.hpp>
#include <shader/shader.hpp>
#include <texture/texture-utils.hpp>
#include <texture/texture2d.hpp>
#include <thread>

#include "menu-layout.hpp"

class LoadingPlayState : public our::State {
    our::TexturedMaterial* loadingMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    bool queuedTransition = false;
    our::TintedMaterial* loadingBarMaterial = nullptr;
    bool assetsFinalized = false;
    std::thread assetLoadingThread;
    std::atomic<bool> assetsLoaded = false;
    int lastLoadingCount = 0;

public:
    void onInitialize(GLFWwindow* loaderWindow) override {
        loadingMaterial = new our::TexturedMaterial();
        loadingMaterial->shader = new our::ShaderProgram();
        loadingMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        loadingMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        loadingMaterial->shader->link();
        loadingMaterial->texture = our::texture_utils::loadImage("assets/textures/menus/loading.png");
        loadingMaterial->tint = glm::vec4(1.0f);

        loadingBarMaterial = new our::TintedMaterial();
        loadingBarMaterial->shader = new our::ShaderProgram();
        loadingBarMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        loadingBarMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        loadingBarMaterial->shader->link();
        loadingBarMaterial->tint = glm::vec4(0.372549f, 0.725490f, 0.274510f, 1.0f);  // color #5ec846

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

        assetsLoaded = false;  // Reset the assets loaded flag in case we return to this state again
        auto config = getApp()->getConfig()["scene"];
        // Load assets in a separate thread to avoid blocking the main thread
        assetLoadingThread = std::thread([this, loaderWindow, config]() {
            glfwMakeContextCurrent(loaderWindow);  // Make the loader window's context current in this thread
            if (config.contains("assets")) {
                our::deserializeAllAssets(config["assets"]);
            }
            glfwMakeContextCurrent(nullptr);  // Detach the context from this thread
            assetsLoaded = true;              // Signal that assets have finished loading
        });
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

        loadingMaterial->setup();
        loadingMaterial->shader->set("transform", VP * menuRect.getLocalToWorld());
        rectangle->draw();
        // place the loading bar at the correct coordinates
        // top left 356/1280, 406/720 -> bottom right 925/1280, 423/720
        float progress = our::AssetLoaderStats::totalCount > 0
                             ? float(our::AssetLoaderStats::loadingCount) / our::AssetLoaderStats::totalCount
                             : 1.0f;
        glm::vec2 barTopLeft = menuRect.localToWorld({356.0f / 1280.0f, 406.0f / 720.0f});
        glm::vec2 barBottomRight = menuRect.localToWorld({925.0f / 1280.0f, 423.0f / 720.0f});
        glm::vec2 barSize = barBottomRight - barTopLeft;
        glm::vec2 progressSize = glm::vec2(barSize.x * progress, barSize.y);

        loadingBarMaterial->setup();
        loadingBarMaterial->shader->set("transform", VP * glm::translate(glm::mat4(1.0f), glm::vec3(barTopLeft, 0.0f)) *
                                                         glm::scale(glm::mat4(1.0f), glm::vec3(progressSize, 1.0f)));
        rectangle->draw();

        if (assetsLoaded) {
            if (assetLoadingThread.joinable()) assetLoadingThread.join();
            getApp()->changeState("play");
        }
    }

    void onDestroy() override {
        if (assetLoadingThread.joinable()) assetLoadingThread.join();
        delete rectangle;
        delete loadingMaterial->texture;
        delete loadingMaterial->shader;
        delete loadingMaterial;
        delete loadingBarMaterial->shader;
        delete loadingBarMaterial;
    }
};
