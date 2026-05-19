#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <algorithm>
#include <application.hpp>
#include <array>
#include <audio/audio-buffer.hpp>
#include <audio/audio-utils.hpp>
#include <cfloat>
#include <iomanip>
#include <material/material.hpp>
#include <menu-layout.hpp>
#include <mesh/mesh-utils.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <sstream>
#include <string>
#include <systems/free-camera-controller.hpp>
#include <texture/texture-utils.hpp>
#include <texture/texture2d.hpp>

namespace gameplay {

    struct PlayOverlayStats {
        float survivalTime = 0.0f;
        int kills = 0;
        int score = 0;
    };

    class PlayOverlay {
        enum class Screen { None, Pause, GameOver };

        our::Application* app = nullptr;
        our::FreeCameraControllerSystem* cameraController = nullptr;

        our::TexturedMaterial* overlayMaterial = nullptr;
        our::TintedMaterial* highlightMaterial = nullptr;
        our::Texture2D* pauseTexture = nullptr;
        our::Texture2D* gameOverTexture = nullptr;
        our::Mesh* rectangle = nullptr;

        std::array<menu_ui::Button, 3> pauseButtons;
        std::array<menu_ui::Button, 3> gameOverButtons;

        our::AudioBuffer* openSound = nullptr;
        our::AudioBuffer* selectSound = nullptr;

        Screen screen = Screen::None;
        bool gameplayAudioPaused = false;
        int hoveredButton = -1;
        float overlayTime = 0.0f;

        static void drawShadowedText(ImDrawList* drawList, const std::string& text, const ImVec2& position, float size,
                                     ImU32 color) {
            ImFont* font = ImGui::GetFont();
            drawList->AddText(font, size, ImVec2(position.x + 2.0f, position.y + 2.0f), IM_COL32(0, 0, 0, 220),
                              text.c_str());
            drawList->AddText(font, size, position, color, text.c_str());
        }

        static std::string formatSurvivalTime(float seconds) {
            int totalSeconds = std::max(0, static_cast<int>(seconds + 0.5f));
            int minutes = totalSeconds / 60;
            int remainder = totalSeconds % 60;

            std::ostringstream stream;
            stream << minutes << ':' << std::setw(2) << std::setfill('0') << remainder;
            return stream.str();
        }

        void setupButtons() {
            glm::vec2 pauseButtonSize = {0.40703125f, 0.08967015f};
            glm::vec2 gameOverButtonSize = {0.31328150f, 0.09522569f};

            pauseButtons[0].normalizedPosition = {0.296875f, 0.3791667f};
            pauseButtons[0].normalizedSize = pauseButtonSize;
            pauseButtons[0].action = [this]() { closePause(); };

            pauseButtons[1].normalizedPosition = {0.296875f, 0.5041667f};
            pauseButtons[1].normalizedSize = pauseButtonSize;
            pauseButtons[1].action = [this]() { app->changeState("menu"); };

            pauseButtons[2].normalizedPosition = {0.296875f, 0.6291667f};
            pauseButtons[2].normalizedSize = pauseButtonSize;
            pauseButtons[2].action = [this]() { app->close(); };

            gameOverButtons[0].normalizedPosition = {0.34375f, 0.6805556f};
            gameOverButtons[0].normalizedSize = gameOverButtonSize;
            gameOverButtons[0].action = [this]() { app->changeState("loading-play"); };

            gameOverButtons[1].normalizedPosition = {0.34375f, 0.7847222f};
            gameOverButtons[1].normalizedSize = gameOverButtonSize;
            gameOverButtons[1].action = [this]() { app->changeState("menu"); };

            gameOverButtons[2].normalizedPosition = {0.34375f, 0.8888889f};
            gameOverButtons[2].normalizedSize = gameOverButtonSize;
            gameOverButtons[2].action = [this]() { app->close(); };
        }

        void openScreen(Screen nextScreen) {
            if (!app || screen == nextScreen) return;

            screen = nextScreen;
            hoveredButton = -1;
            overlayTime = 0.0f;

#ifdef __EMSCRIPTEN__
            if (cameraController && nextScreen != Screen::Pause) cameraController->exit();
            EM_ASM({ window.__arenaOverlayActive = true; });
#else
            if (cameraController) cameraController->exit();
#endif

            if (nextScreen == Screen::Pause && !gameplayAudioPaused) {
                app->getAudioSystem().pauseAll();
                gameplayAudioPaused = true;
            }

            if (openSound) app->getAudioSystem().playSound2D(openSound, 0.9f, 1.0f, false);
        }

        template <size_t N>
        int getHoveredButton(const std::array<menu_ui::Button, N>& buttons, const menu_ui::PixelRect& menuRect,
                             const glm::vec2& mousePosition) const {
            for (int i = 0; i < static_cast<int>(buttons.size()); i++)
                if (buttons[i].getPixelRect(menuRect).isInside(mousePosition)) return i;

            return -1;
        }

        template <size_t N>
        void handleClick(const std::array<menu_ui::Button, N>& buttons, const menu_ui::PixelRect& menuRect,
                         const glm::vec2& mousePosition) {
            auto& mouse = app->getMouse();
            if (!mouse.justPressed(GLFW_MOUSE_BUTTON_LEFT)) return;

            for (auto& button : buttons) {
                if (button.getPixelRect(menuRect).isInside(mousePosition)) {
                    button.action();
                    break;
                }
            }
        }

        template <size_t N>
        void renderScreen(our::Texture2D* texture, const std::array<menu_ui::Button, N>& buttons, double deltaTime) {
            if (!(app && overlayMaterial && overlayMaterial->shader && highlightMaterial && highlightMaterial->shader &&
                  rectangle))
                return;

            glm::ivec2 size = app->getFrameBufferSize();
            menu_ui::PixelRect menuRect = menu_ui::getMenuRect(size);
            glm::vec2 mousePosition = menu_ui::getMousePositionInTarget(app, size);

            glViewport(0, 0, size.x, size.y);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 VP = glm::ortho(0.0f, static_cast<float>(size.x), static_cast<float>(size.y), 0.0f, 1.0f, -1.0f);
            glm::mat4 M = menuRect.getLocalToWorld();

            overlayTime += static_cast<float>(deltaTime);
            overlayMaterial->texture = texture;
            overlayMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 0.25f, overlayTime));
            overlayMaterial->setup();
            overlayMaterial->shader->set("transform", VP * M);
            rectangle->draw();

            int currentlyHovered = getHoveredButton(buttons, menuRect, mousePosition);
            if (currentlyHovered != -1 && currentlyHovered != hoveredButton && selectSound)
                app->getAudioSystem().playSound2D(selectSound, 1.0f, 1.0f, false);

            hoveredButton = currentlyHovered;

            if (currentlyHovered != -1) {
                menu_ui::PixelRect hoveredRect = buttons[currentlyHovered].getPixelRect(menuRect);
                highlightMaterial->setup();
                highlightMaterial->shader->set("transform", VP * hoveredRect.getLocalToWorld());
                rectangle->draw();
            }
        }

    public:
        void initialize(our::Application* app, our::FreeCameraControllerSystem* cameraController) {
            destroy();

            this->app = app;
            this->cameraController = cameraController;

            overlayMaterial = new our::TexturedMaterial();
            overlayMaterial->shader = new our::ShaderProgram();
            overlayMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            overlayMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            overlayMaterial->shader->link();
            overlayMaterial->tint = glm::vec4(1.0f);

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

            pauseTexture = our::texture_utils::loadImage("assets/textures/menus/pause.png");
            gameOverTexture = our::texture_utils::loadImage("assets/textures/menus/game-over.png");
            openSound = our::audio_utils::loadWAV("assets/sounds/menu-open.wav");
            selectSound = our::audio_utils::loadWAV("assets/sounds/menu-select.wav");

            setupButtons();
            reset();
        }

        void destroy() {
            delete rectangle;
            delete pauseTexture;
            delete gameOverTexture;

            if (overlayMaterial) delete overlayMaterial->shader;
            delete overlayMaterial;

            if (highlightMaterial) delete highlightMaterial->shader;
            delete highlightMaterial;

            delete openSound;
            delete selectSound;

            rectangle = nullptr;
            pauseTexture = nullptr;
            gameOverTexture = nullptr;
            overlayMaterial = nullptr;
            highlightMaterial = nullptr;
            openSound = nullptr;
            selectSound = nullptr;

            app = nullptr;
            cameraController = nullptr;
            reset();
        }

        void reset() {
            screen = Screen::None;
            gameplayAudioPaused = false;
            hoveredButton = -1;
            overlayTime = 0.0f;
#ifdef __EMSCRIPTEN__
            EM_ASM({
                window.__arenaOverlayActive = false;
                window.__arenaPauseRequested = false;
            });
#endif
        }

        bool isActive() const {
            return screen != Screen::None;
        }

        void openPause() {
            openScreen(Screen::Pause);
        }

        void openGameOver() {
            openScreen(Screen::GameOver);
        }

        void closePause() {
            screen = Screen::None;
            hoveredButton = -1;
            overlayTime = 0.0f;

            if (app && gameplayAudioPaused) {
                app->getAudioSystem().resumeAll();
                gameplayAudioPaused = false;
            }

#ifdef __EMSCRIPTEN__
            EM_ASM({ window.__arenaOverlayActive = false; });
#endif

            if (app && cameraController) {
                cameraController->enter(app);
            }
        }

        void drawImmediateGui(const PlayOverlayStats& stats) const {
            if (!(app && screen == Screen::GameOver)) return;

            menu_ui::PixelRect menuRect = menu_ui::getMenuRect(app->getWindowSize());
            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            ImFont* font = ImGui::GetFont();
            float fontSize = 34.0f * (menuRect.size.y / 720.0f);
            float rightEdge = menuRect.position.x + menuRect.size.x * (940.0f / 1280.0f);
            float startY = menuRect.position.y + menuRect.size.y * (278.0f / 720.0f);
            float rowStep = menuRect.size.y * (60.0f / 720.0f);

            std::array<std::string, 3> values = {
                std::to_string(stats.score),
                std::to_string(stats.kills),
                formatSurvivalTime(stats.survivalTime),
            };

            for (int i = 0; i < static_cast<int>(values.size()); i++) {
                const std::string& value = values[i];
                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, value.c_str());
                ImVec2 position(rightEdge - textSize.x, startY + rowStep * i);
                drawShadowedText(drawList, value, position, fontSize, IM_COL32(255, 220, 220, 255));
            }
        }

        bool handleActiveFrame(double deltaTime) {
            if (!app) return false;

            our::Keyboard& keyboard = app->getKeyboard();
            our::Joystick& joystick = app->getJoystick();
            if (screen == Screen::Pause) {
                if (keyboard.justPressed(GLFW_KEY_ESCAPE) || joystick.justPressed(GLFW_GAMEPAD_BUTTON_START) ||
                    joystick.justPressed(GLFW_GAMEPAD_BUTTON_A)) {
                    closePause();
                    return true;
                }

                if (joystick.justPressed(GLFW_GAMEPAD_BUTTON_B)) {
                    app->changeState("menu");
                    return true;
                }

                glm::ivec2 size = app->getFrameBufferSize();
                menu_ui::PixelRect menuRect = menu_ui::getMenuRect(size);
                glm::vec2 mousePosition = menu_ui::getMousePositionInTarget(app, size);
                handleClick(pauseButtons, menuRect, mousePosition);
                if (screen != Screen::Pause) return true;

                renderScreen(pauseTexture, pauseButtons, deltaTime);
                return true;
            }

            if (screen == Screen::GameOver) {
                if (keyboard.justPressed(GLFW_KEY_ESCAPE) || joystick.justPressed(GLFW_GAMEPAD_BUTTON_B)) {
                    app->changeState("menu");
                    return true;
                }

                if (joystick.justPressed(GLFW_GAMEPAD_BUTTON_A)) {
                    app->changeState("loading-play");
                    return true;
                }

                glm::ivec2 size = app->getFrameBufferSize();
                menu_ui::PixelRect menuRect = menu_ui::getMenuRect(size);
                glm::vec2 mousePosition = menu_ui::getMousePositionInTarget(app, size);
                handleClick(gameOverButtons, menuRect, mousePosition);
                renderScreen(gameOverTexture, gameOverButtons, deltaTime);
                return true;
            }

            return false;
        }

        void renderCurrent(double deltaTime) {
            if (screen == Screen::Pause)
                renderScreen(pauseTexture, pauseButtons, deltaTime);
            else if (screen == Screen::GameOver)
                renderScreen(gameOverTexture, gameOverButtons, deltaTime);
        }
    };

}  // namespace gameplay
