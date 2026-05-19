#pragma once

#include <application.hpp>
#include <asset-loader.hpp>
#include <atomic>
#include <glm/common.hpp>
#include <material/material.hpp>
#include <mesh/mesh-utils.hpp>
#include <mesh/mesh.hpp>
#include <model/model.hpp>
#include <shader/shader.hpp>
#include <texture/texture-utils.hpp>
#include <texture/texture2d.hpp>
#ifndef __EMSCRIPTEN__
#include <thread>
#endif

#include "menu-layout.hpp"

class LoadingPlayState : public our::State {
    our::TexturedMaterial* loadingMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    bool queuedTransition = false;
    our::TintedMaterial* loadingBarMaterial = nullptr;
    bool assetsFinalized = false;
#ifndef __EMSCRIPTEN__
    std::thread assetLoadingThread;
#endif
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

        rectangle = our::mesh_utils::generateQuad();

        assetsLoaded = false;  // Reset the assets loaded flag in case we return to this state again
        auto config = getApp()->getConfig()["scene"];
#ifdef __EMSCRIPTEN__
        // WebGL contexts cannot be shared with std::thread on the default Emscripten build,
        // so load synchronously on the main thread. The progress bar will jump straight to full.
        if (config.contains("assets")) {
            our::deserializeAllAssets(config["assets"]);
        }
        assetsLoaded = true;
#else
        // Load assets in a separate thread to avoid blocking the main thread
        assetLoadingThread = std::thread([this, loaderWindow, config]() {
            glfwMakeContextCurrent(loaderWindow);  // Make the loader window's context current in this thread
            if (config.contains("assets")) {
                our::deserializeAllAssets(config["assets"]);
            }
            glfwMakeContextCurrent(nullptr);  // Detach the context from this thread
            assetsLoaded = true;              // Signal that assets have finished loading
        });
#endif
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
#ifndef __EMSCRIPTEN__
            if (assetLoadingThread.joinable()) assetLoadingThread.join();
#endif
            getApp()->changeState("play");
        }
    }

    void onDestroy() override {
#ifndef __EMSCRIPTEN__
        if (assetLoadingThread.joinable()) assetLoadingThread.join();
#endif
        delete rectangle;
        delete loadingMaterial->texture;
        delete loadingMaterial->shader;
        delete loadingMaterial;
        delete loadingBarMaterial->shader;
        delete loadingBarMaterial;
    }
};
