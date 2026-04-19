#pragma once

#include <application.hpp>
#include <array>
#include <asset-loader.hpp>
#include <audio/audio-buffer.hpp>
#include <audio/audio-utils.hpp>
#include <functional>
#include <glm/common.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <texture/texture-utils.hpp>
#include <texture/texture2d.hpp>

// This struct is used to store the location and size of a button and the code it should execute when clicked
struct PixelRect {
    // The position (of the top-left corner) of the button and its size in pixels
    glm::vec2 position, size;

    // This function returns true if the given vector v is inside the button. Otherwise, false is returned.
    // This is used to check if the mouse is hovering over the button.
    bool isInside(const glm::vec2& v) const {
        return position.x <= v.x && position.y <= v.y && v.x <= position.x + size.x && v.y <= position.y + size.y;
    }

    // This function returns the local to world matrix to transform a rectangle of size 1x1
    // (and whose top-left corner is at the origin) to be the button.
    glm::mat4 getLocalToWorld() const {
        return glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
    }
};

// Button using normalized coordinates.
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

// This state shows how to use some of the abstractions we created to make a menu.
class Menustate : public our::State {
    // A meterial holding the menu shader and the menu texture to draw
    our::TexturedMaterial* menuMaterial;
    // A material to be used to highlight hovered buttons (we will use blending to create a negative effect).
    our::TintedMaterial* highlightMaterial;
    // A rectangle mesh on which the menu material will be drawn
    our::Mesh* rectangle;
    // A variable to record the time since the state is entered (it will be used for the fading effect).
    float time;
    // An array of the button that we can interact with
    std::array<Button, 2> buttons;

    static PixelRect getMenuRect(glm::ivec2 framebufferSize) {
        glm::vec2 viewportSize(framebufferSize);
        constexpr float menuAspectRatio = 16.0f / 9.0f;

        glm::vec2 contentSize = viewportSize;
        if (viewportSize.x / viewportSize.y > menuAspectRatio) {
            contentSize.x = viewportSize.y * menuAspectRatio;
        } else {
            contentSize.y = viewportSize.x / menuAspectRatio;
        }

        return {(viewportSize - contentSize) * 0.5f, contentSize};
    }

    glm::vec2 getMousePositionInFramebuffer(glm::ivec2 framebufferSize) {
        glm::vec2 mousePosition = getApp()->getMouse().getMousePosition();
        glm::ivec2 windowSize = getApp()->getWindowSize();
        if (windowSize.x <= 0 || windowSize.y <= 0) return mousePosition;

        return {
            mousePosition.x * framebufferSize.x / static_cast<float>(windowSize.x),
            mousePosition.y * framebufferSize.y / static_cast<float>(windowSize.y),
        };
    }

    our::AudioBuffer* menuMusic = nullptr;
    our::AudioBuffer* menuSelectSound = nullptr;

    // Index of the currently hovered button (-1 = none). Used to detect hover-enter transitions.
    int hoveredButton = -1;

    void onInitialize() override {
        // First, we create a material for the menu's background
        menuMaterial = new our::TexturedMaterial();
        // Here, we load the shader that will be used to draw the background
        menuMaterial->shader = new our::ShaderProgram();
        menuMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        menuMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        menuMaterial->shader->link();
        // Then we load the menu texture
        menuMaterial->texture = our::texture_utils::loadImage("assets/textures/menu.png");
        // Initially, the menu material will be black, then it will fade in
        menuMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        // Second, we create a material to highlight the hovered buttons
        highlightMaterial = new our::TintedMaterial();
        // Since the highlight is not textured, we used the tinted material shaders
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        // The tint is white since we will subtract the background color from it to create a negative effect.
        highlightMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        // To create a negative effect, we enable blending, set the equation to be subtract,
        // and set the factors to be one for both the source and the destination.
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_SUBTRACT;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_ONE;
        highlightMaterial->pipelineState.blending.destinationFactor = GL_ONE;

        // Then we create a rectangle whose top-left corner is at the origin and its size is 1x1.
        // Note that the texture coordinates at the origin is (0.0, 1.0) since we will use the
        // projection matrix to make the origin at the the top-left corner of the screen.
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

        // Reset the time elapsed since the state is entered.
        time = 0;

        // Fill the positions, sizes and actions for the menu buttons
        // Note that we use lambda expressions to set the actions of the buttons.
        // A lambda expression consists of 3 parts:
        // - The capture list [] which is the variables that the lambda should remember because it will use them during
        // execution.
        //      We store [this] in the capture list since we will use it in the action.
        // - The argument list () which is the arguments that the lambda should receive when it is called.
        //      We leave it empty since button actions receive no input.
        // - The body {} which contains the code to be executed.
        buttons[0].normalizedPosition = {0.6484375f, 0.8430556f};
        buttons[0].normalizedSize = {0.3125f, 0.0458333f};
        buttons[0].action = [this]() { this->getApp()->changeState("play"); };

        buttons[1].normalizedPosition = {0.6484375f, 0.8944444f};
        buttons[1].normalizedSize = {0.3125f, 0.0458333f};
        buttons[1].action = [this]() { this->getApp()->close(); };

        // TODO: Maybe load them in application.cpp or serialize them from the config json
        menuMusic = our::audio_utils::loadWAV("assets/sounds/menu-music.wav");
        menuSelectSound = our::audio_utils::loadWAV("assets/sounds/menu-select.wav");
        getApp()->getAudioSystem().playSound2D(menuMusic, 0.5f, 1.0f, true);
    }

    void onDraw(double deltaTime) override {
        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if (keyboard.justPressed(GLFW_KEY_SPACE)) {
            // If the space key is pressed in this frame, go to the play state
            getApp()->changeState("play");
        } else if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape key is pressed in this frame, exit the game
            getApp()->close();
        }

        glm::ivec2 size = getApp()->getFrameBufferSize();
        PixelRect menuRect = getMenuRect(size);

        // Get a reference to the mouse object and convert the current mouse position to framebuffer coordinates.
        auto& mouse = getApp()->getMouse();
        glm::vec2 mousePosition = getMousePositionInFramebuffer(size);

        // If the mouse left-button is just pressed, check if the mouse was inside
        // any menu button. If it was inside a menu button, run the action of the button.
        if (mouse.justPressed(0)) {
            for (auto& button : buttons) {
                if (button.getPixelRect(menuRect).isInside(mousePosition)) button.action();
            }
        }

        // Make sure the viewport covers the whole size of the framebuffer.
        glViewport(0, 0, size.x, size.y);

        // Clear to black so any space around the aspect-preserved menu image stays black
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // The view matrix is an identity (there is no camera that moves around).
        // The projection matrix applys an orthographic projection whose size is the framebuffer size in pixels
        // so that the we can define our object locations and sizes in pixels.
        // Note that the top is at 0.0 and the bottom is at the framebuffer height. This allows us to consider the
        // top-left corner of the window to be the origin which makes dealing with the mouse input easier.
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        // The local to world (model) matrix of the background draws the menu inside an aspect-preserved content area.
        glm::mat4 M = menuRect.getLocalToWorld();

        // First, we apply the fading effect.
        time += (float)deltaTime;
        menuMaterial->tint = glm::vec4(glm::smoothstep(0.00f, 2.00f, time));
        // Then we render the menu background
        menuMaterial->setup();
        menuMaterial->shader->set("transform", VP * M);
        rectangle->draw();

        // Determine which button (if any) the mouse is currently over
        int currentlyHovered = -1;
        for (int i = 0; i < (int)buttons.size(); i++) {
            if (buttons[i].getPixelRect(menuRect).isInside(mousePosition)) {
                currentlyHovered = i;
                break;
            }
        }

        // Play hover sound only on the frame the mouse first enters a button (edge detection)
        if (currentlyHovered != -1 && currentlyHovered != hoveredButton) {
            getApp()->getAudioSystem().playSound2D(menuSelectSound, 1.0, 1.0, false);
        }
        hoveredButton = currentlyHovered;

        // Draw the highlight rectangle over the hovered button
        if (currentlyHovered != -1) {
            PixelRect hoveredRect = buttons[currentlyHovered].getPixelRect(menuRect);
            highlightMaterial->setup();
            highlightMaterial->shader->set("transform", VP * hoveredRect.getLocalToWorld());
            rectangle->draw();
        }
    }

    void onDestroy() override {
        // Delete all the allocated resources
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
