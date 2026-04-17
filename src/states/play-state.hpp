#pragma once

#include <application.hpp>
#include <components/player.hpp>
#include <ecs/world.hpp>
#include <systems/audio-system.hpp>
#include <systems/enemy-ai.hpp>
#include <systems/enemy-spawner.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>

// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State {
    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    ALuint reloadSource = 0;  // TODO: remove when implementing a proper weapon/player system
    our::Entity* playerEntity = nullptr;
    gameplay::EnemyAISystem enemyAI;
    gameplay::EnemySpawner enemySpawner;

    void displayFPS() const {
        // Pin a transparent overlay window to the top-left corner
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);

        // Red if below 120 FPS, green otherwise
        float fps = ImGui::GetIO().Framerate;
        ImVec4 fpsColor = (fps < 120.0f) ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

        ImGuiWindowFlags textWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                           ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("FPS Overlay", nullptr, textWindowFlags)) {
            ImGui::TextColored(fpsColor, "FPS: %.1f", fps);
        }
        ImGui::End();
    }

    void onImmediateGui() override {
        displayFPS();
    }

    void onInitialize() override {
        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if (config.contains("assets")) {
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if (config.contains("world")) {
            world.deserialize(config["world"]);
        }

        // Store a pointer to the player entity
        for (our::Entity* entity : world.getEntities()) {
            if (entity->getComponent<gameplay::PlayerComponent>()) {
                playerEntity = entity;
                break;
            }
        }

        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);

        enemySpawner.deserialize(config);
        enemySpawner.initialize(&world);
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        getApp()->getAudioSystem().update(&world);
        enemyAI.update(&world, playerEntity, (float)deltaTime);
        enemySpawner.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
        // clang-format off
        if (keyboard.justPressed(GLFW_KEY_R)) {
            if (!getApp()->getAudioSystem().isPlaying(reloadSource)) { // TODO: remove when implementing a proper weapon/player system
                reloadSource = getApp()->getAudioSystem().playSound2D(
                    our::AssetLoader<our::AudioBuffer>::get("gun-reload"), 1.0f, 1.0f, false);
            }
        }
        // clang-format on
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Stop all audio and release resources
        getApp()->getAudioSystem().stopAll();
        playerEntity = nullptr;
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};
