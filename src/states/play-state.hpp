#pragma once

#include <application.hpp>
#include <components/camera.hpp>
#include <components/enemy.hpp>
#include <components/health.hpp>
#include <components/player.hpp>
#include <ecs/world.hpp>
#include <systems/animation-system.hpp>
#include <systems/audio-system.hpp>
#include <systems/collision-system.hpp>
#include <systems/crosshair-renderer.hpp>
#include <systems/enemy-ai.hpp>
#include <systems/enemy-health-bar.hpp>
#include <systems/enemy-spawner.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/health-system.hpp>
#include <systems/movement.hpp>
#include <systems/player-movement-system.hpp>
#include <systems/post-process-effects-system.hpp>
#include <systems/projectile-system.hpp>
#include <systems/trail-system.hpp>
#include <systems/ui-renderer.hpp>
#include <systems/weapon-switcher-system.hpp>
#include <systems/weapon-visual-system.hpp>
#include <ui/play-overlay.hpp>

#include "../game/systems/player-hud.hpp"

class Playstate : public our::State {
    our::World world;
    our::ForwardRenderer renderer;
    our::UIRenderer uiRenderer;
    our::TextRenderer textRenderer;
    our::AnimationSystem animationSystem;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;

    gameplay::CollisionSystem collisionSystem;
    gameplay::HealthSystem healthSystem;
    our::Entity* playerEntity = nullptr;
    our::CameraComponent* activeCamera = nullptr;
    gameplay::EnemyAISystem enemyAI;
    gameplay::EnemySpawner enemySpawner;
    gameplay::EnemyHealthBarSystem enemyHealthBars;
    gameplay::PlayerMovementSystem playerMovement;
    gameplay::PostProcessEffectsSystem postProcessEffects;
    gameplay::CrosshairRenderer crosshair;
    gameplay::WeaponVisualSystem weaponVisuals;
    float aimBlend = 0.0f;
    gameplay::PlayOverlay overlay;
    gameplay::PlayOverlayStats overlayStats;
    gameplay::PlayerHUDSystem playerHud;
    gameplay::WeaponSwitcherSystem weaponSwitcher;

    void displayFPS() const {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);

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

public:
    void onImmediateGui() override {
        if (!overlay.isActive()) {
            displayFPS();

#ifdef COLLISION_DEBUG_DRAW
            if (collisionSystem.isDebugDrawEnabled()) {
                ImGui::SetNextWindowPos(ImVec2(10.0f, 40.0f), ImGuiCond_Always);
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
                ImGui::SetNextWindowBgAlpha(0.35f);
                if (ImGui::Begin("Debug Draw Overlay", nullptr, flags)) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[F3] Collision Debug: ON");
                }
                ImGui::End();
            }
#endif
        } else {
            overlay.drawImmediateGui(overlayStats);
        }
    }

    void onInitialize(GLFWwindow*) override {
        auto& config = getApp()->getConfig()["scene"];
        if (config.contains("world")) {
            world.deserialize(config["world"]);
        }

        activeCamera = nullptr;
        playerEntity = nullptr;
        for (our::Entity* entity : world.getEntities()) {
            if (!activeCamera) activeCamera = entity->getComponent<our::CameraComponent>();
            if (!playerEntity && entity->getComponent<gameplay::PlayerComponent>()) playerEntity = entity;
            if (activeCamera && playerEntity) break;
        }

        weaponVisuals.initlize(playerEntity, activeCamera ? activeCamera->getOwner() : nullptr);

        cameraController.enter(getApp());
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
        uiRenderer.initialize();
        textRenderer.initialize();
        collisionSystem.initialize();
        overlay.initialize(getApp(), &cameraController);
        if (config.contains("hud")) playerHud.deserialize(config["hud"]);
        playerHud.initialize();

        enemySpawner.deserialize(config);
        enemySpawner.initialize(&world);
        collisionSystem.update(&world);  // make sure spawner colliders are registered before enemyHealthBars
        enemyHealthBars.deserialize(config);
        crosshair.deserialize(config);
        overlayStats = {};
        weaponSwitcher.initialize(&world, playerEntity);
    }

    void onDraw(double deltaTime) override {
        float dt = static_cast<float>(deltaTime);
        our::Keyboard& keyboard = getApp()->getKeyboard();

        if (overlay.handleActiveFrame(deltaTime)) return;

        if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            overlay.openPause();
            overlay.renderCurrent(deltaTime);
            return;
        }

        overlayStats.survivalTime += dt;

        gameplay::ProjectileSystem::tickWeaponCooldowns(&world, dt);

        // Movement / Camera
        cameraController.update(&world, dt);
        playerMovement.update(&world, dt, getApp(), &collisionSystem);
        movementSystem.update(&world, dt);

        glm::vec3 playerVelocity = glm::vec3(0.0f);
        bool playerOnGround = true;
        if (playerEntity) {
            if (gameplay::PlayerMovementComponent* movement =
                    playerEntity->getComponent<gameplay::PlayerMovementComponent>()) {
                playerVelocity = movement->velocity;
                playerOnGround = movement->isGrounded;
            }
        }

        // Spawning / AI
        bool waveCompleted = enemySpawner.update(&world, dt);
        if (waveCompleted && playerEntity) {
            auto reward = enemySpawner.getWaveReward();

            gameplay::HealthComponent* health = playerEntity->getComponent<gameplay::HealthComponent>();
            if (health) health->currentHealth = std::min(health->currentHealth + reward.health, health->maxHealth);

            gameplay::PlayerComponent* player = playerEntity->getComponent<gameplay::PlayerComponent>();
            if (player && player->currentWeapon) {
                gameplay::WeaponComponent* weapon = player->currentWeapon;
                weapon->maxAmmo = std::min(weapon->maxAmmo + reward.ammo, 999);
            }
        }

        enemyAI.update(&world, playerEntity, getApp(), dt, &collisionSystem);

        // Keep collision queries in sync with all movement before shooting/projectiles.
        collisionSystem.update(&world);

        // handle weapon swtiching
        weaponSwitcher.update(getApp());

        bool reloadStarted = gameplay::ProjectileSystem::handlePlayerReload(getApp(), playerEntity);
        if (reloadStarted) {
            float reloadDuration = 1.0f;
            if (playerEntity) {
                gameplay::PlayerComponent* player = playerEntity->getComponent<gameplay::PlayerComponent>();
                if (player && player->currentWeapon) {
                    reloadDuration = player->currentWeapon->reloadTime;
                }
            }
            weaponVisuals.onReloadStart(reloadDuration);
        }

        bool fired = gameplay::ProjectileSystem::handlePlayerFire(&world, getApp(), collisionSystem, playerEntity);
        if (fired) weaponVisuals.onFire();

        gameplay::ProjectileSystem::update(&world, collisionSystem, dt);
        gameplay::TrailSystem::update(&world, dt);

        // Death / effects / audio
        gameplay::HealthUpdateResult healthResult = healthSystem.update(&world, dt);
        overlayStats.kills += healthResult.kills;
        overlayStats.score += healthResult.score;

        // apply the rewards
        if (playerEntity && (healthResult.healthReward != 0.0f || healthResult.ammoReward != 0)) {
            gameplay::HealthComponent* health = playerEntity->getComponent<gameplay::HealthComponent>();
            if (health)
                health->currentHealth = std::min(health->currentHealth + healthResult.healthReward, health->maxHealth);

            gameplay::PlayerComponent* player = playerEntity->getComponent<gameplay::PlayerComponent>();
            if (player && player->currentWeapon) {
                gameplay::WeaponComponent* weapon = player->currentWeapon;
                weapon->maxAmmo = std::min(weapon->maxAmmo + healthResult.ammoReward, 999);
            }
        }

        world.deleteMarkedEntities();
        if (healthResult.playerDied) {
            overlay.openGameOver();
            overlay.renderCurrent(deltaTime);
            return;
        }

        postProcessEffects.update(&world, dt);
        weaponVisuals.update(&world, dt, playerVelocity, playerOnGround, cameraController.isAiming());
        animationSystem.update(&world, dt);
        getApp()->getAudioSystem().update(&world);

        // Rendering
        renderer.render(&world, getApp()->getFrameBufferSize());
        enemyHealthBars.render(&world, getApp(), uiRenderer, activeCamera, collisionSystem);
        playerHud.render(&world, playerEntity, getApp()->getFrameBufferSize(), &textRenderer, enemySpawner);

        // HUD
        float aimTarget = cameraController.isAiming() ? 1.0f : 0.0f;
        float aimSpeed = cameraController.getAimSpeed();
        aimBlend = glm::mix(aimBlend, aimTarget, glm::clamp(aimSpeed * dt, 0.0f, 1.0f));
        crosshair.render(getApp(), uiRenderer, aimBlend);
#ifdef COLLISION_DEBUG_DRAW
        if (keyboard.justPressed(GLFW_KEY_F3)) {
            collisionSystem.setDebugDrawEnabled(!collisionSystem.isDebugDrawEnabled());
        }

        if (collisionSystem.isDebugDrawEnabled()) {
            if (activeCamera) {
                auto size = getApp()->getFrameBufferSize();
                glm::mat4 VP = activeCamera->getProjectionMatrix(size) * activeCamera->getViewMatrix();
                collisionSystem.debugDraw(VP);
            }
        }
#endif
    }

    void onDestroy() override {
        collisionSystem.destroy();
        renderer.destroy();
        uiRenderer.destroy();
        textRenderer.destroy();
        playerHud.destroy();
        weaponVisuals.destroy();
        weaponSwitcher.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        getApp()->getAudioSystem().stopAll();
        overlay.destroy();
        activeCamera = nullptr;
        playerEntity = nullptr;
        world.clear();
    }
};
