#pragma once

#include <GLFW/glfw3.h>

#include <cstring>
#include <glm/glm.hpp>
#include <iomanip>
#include <iostream>

namespace our {

    class Joystick {
    private:
        bool enabled;
        bool currentButtons[GLFW_GAMEPAD_BUTTON_LAST + 1], previousButtons[GLFW_GAMEPAD_BUTTON_LAST + 1];
        float currentAxes[GLFW_GAMEPAD_AXIS_LAST + 1], previousAxes[GLFW_GAMEPAD_AXIS_LAST + 1];
        int currentJoystickId = -1;
        bool currentTriggersPressed[2] = {false, false}, previousTriggersPressed[2] = {false, false};

    public:
        Joystick() : enabled(false) {
            disable();
        }

        int getFirstGamepad() {
            for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++) {
                if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i)) return i;
            }
            return -1;  // none connected
        }

        void enable() {
            enabled = true;
            std::memset(currentButtons, 0, sizeof(currentButtons));
            std::memset(previousButtons, 0, sizeof(previousButtons));
            std::memset(currentAxes, 0, sizeof(currentAxes));
            std::memset(previousAxes, 0, sizeof(previousAxes));
            currentJoystickId = getFirstGamepad();
            if (currentJoystickId == -1) return;
            GLFWgamepadstate state;
            if (glfwGetGamepadState(currentJoystickId, &state)) {
                for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; i++)
                    currentButtons[i] = previousButtons[i] = state.buttons[i] == GLFW_PRESS;
                for (int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; i++) currentAxes[i] = previousAxes[i] = state.axes[i];
                currentTriggersPressed[0] = previousTriggersPressed[0] =
                    state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.01f - 1;
                currentTriggersPressed[1] = previousTriggersPressed[1] =
                    state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.01f - 1;
            }
        }

        void disable() {
            enabled = false;
            currentJoystickId = -1;
            std::memset(currentButtons, 0, sizeof(currentButtons));
            std::memset(previousButtons, 0, sizeof(previousButtons));
            std::memset(currentAxes, 0, sizeof(currentAxes));
            std::memset(previousAxes, 0, sizeof(previousAxes));
        }

        void update() {
            if (!enabled) return;
            std::memcpy(previousButtons, currentButtons, sizeof(previousButtons));
            std::memcpy(previousAxes, currentAxes, sizeof(previousAxes));
            std::memcpy(previousTriggersPressed, currentTriggersPressed, sizeof(previousTriggersPressed));
            GLFWgamepadstate state;
            if (currentJoystickId == -1) {
                currentJoystickId = getFirstGamepad();
                if (currentJoystickId == -1) {
                    std::memset(currentButtons, 0, sizeof(currentButtons));
                    std::memset(currentAxes, 0, sizeof(currentAxes));
                    std::memset(currentTriggersPressed, 0, sizeof(currentTriggersPressed));
                    return;
                }
            }

            if (glfwGetGamepadState(currentJoystickId, &state)) {
                for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; i++) currentButtons[i] = state.buttons[i] == GLFW_PRESS;
                for (int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; i++) currentAxes[i] = state.axes[i];
                currentTriggersPressed[0] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.01f - 1;
                currentTriggersPressed[1] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.01f - 1;
            } else {
                // Joystick disconnected mid-session — clear current but keep previous for justReleased to fire
                std::memset(currentButtons, 0, sizeof(currentButtons));
                std::memset(currentAxes, 0, sizeof(currentAxes));
                std::memset(currentTriggersPressed, 0, sizeof(currentTriggersPressed));
            }
        }

        void joystickEvent(int jid, int event) {
            if (event == GLFW_DISCONNECTED && jid == currentJoystickId && enabled) {
                enabled = false;  // Automatically disable if the joystick is disconnected
                currentJoystickId = -1;
                std::memset(currentButtons, 0, sizeof(currentButtons));
                std::memset(currentAxes, 0, sizeof(currentAxes));
                std::memset(currentTriggersPressed, 0, sizeof(currentTriggersPressed));
            } else if (event == GLFW_CONNECTED && !enabled) {
                enable();  // Automatically enable if the joystick is connected
            }
        }

        [[nodiscard]] bool isPressed(int button) const {
            return currentButtons[button];
        }
        [[nodiscard]] bool justPressed(int button) const {
            return currentButtons[button] && !previousButtons[button];
        }
        [[nodiscard]] bool justReleased(int button) const {
            return !currentButtons[button] && previousButtons[button];
        }
        [[nodiscard]] float getAxis(int axis) const {
            return currentAxes[axis];
        }

        [[nodiscard]] bool isTriggerPressed(int triggerIndex) const {
            return currentTriggersPressed[triggerIndex];
        }
        [[nodiscard]] bool justPressedTrigger(int triggerIndex) const {
            return currentTriggersPressed[triggerIndex] && !previousTriggersPressed[triggerIndex];
        }
        [[nodiscard]] bool justReleasedTrigger(int triggerIndex) const {
            return !currentTriggersPressed[triggerIndex] && previousTriggersPressed[triggerIndex];
        }

        [[nodiscard]] glm::vec2 getLeftStick() const {
            return {getAxis(GLFW_GAMEPAD_AXIS_LEFT_X), getAxis(GLFW_GAMEPAD_AXIS_LEFT_Y)};
        }
        [[nodiscard]] glm::vec2 getRightStick() const {
            return {getAxis(GLFW_GAMEPAD_AXIS_RIGHT_X), getAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y)};
        }
        [[nodiscard]] float getLeftTrigger() const {
            return (getAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER) + 1.0f) / 2.0f;
        }
        [[nodiscard]] float getRightTrigger() const {
            return (getAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER) + 1.0f) / 2.0f;
        }

        [[nodiscard]] bool isEnabled() const {
            return enabled;
        }
        void setEnabled(bool enabled) {
            if (this->enabled != enabled) {
                if (enabled)
                    enable();
                else
                    disable();
            }
        }
    };

}  // namespace our
