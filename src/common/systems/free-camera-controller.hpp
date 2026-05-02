#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/trigonometric.hpp>

#include "../application.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../ecs/world.hpp"

namespace our {

    // The free camera controller system is responsible for moving every entity which contains a
    // FreeCameraControllerComponent. This system is added as a slightly complex example for how use the ECS framework
    // to implement logic. For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem {
        Application* app;           // The application in which the state runs
        bool mouse_locked = false;  // Is the mouse locked
        float baseFov = 0.0f;
        bool aiming = false;
        float currentAimSpeed = 8.0f;

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app) {
            this->app = app;
            app->getMouse().lockMouse(app->getWindow());
            app->getMouse().enable(app->getWindow());
            mouse_locked = true;
            aiming = false;
            baseFov = 0.0f;
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent
        void update(World* world, float deltaTime) {
            // First of all, we search for an entity containing both a CameraComponent and a
            // FreeCameraControllerComponent As soon as we find one, we break
            CameraComponent* camera = nullptr;
            FreeCameraControllerComponent* controller = nullptr;
            for (auto entity : world->getEntities()) {
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if (camera && controller) break;
            }
            // If there is no entity with both a CameraComponent and a FreeCameraControllerComponent, we can do nothing
            // so we return
            if (!(camera && controller)) return;
            // Get the entity that we found via getOwner of camera (we could use controller->getOwner())
            Entity* entity = camera->getOwner();

            // We get a reference to the entity's rotation
            glm::vec3& rotation = entity->localTransform.rotation;

            glm::vec2 delta{0.0f};
            // While the play state is active, the mouse stays locked
            if (mouse_locked) {
                delta = app->getMouse().getMouseDelta();
                rotation.x -= delta.y * controller->rotationSensitivity;  // The y-axis controls the pitch
                rotation.y -= delta.x * controller->rotationSensitivity;  // The x-axis controls the yaw
            }

            delta = app->getJoystick().getRightStick();
            if (glm::length(delta) > 0.1f) {                                // deadzone
                rotation.x -= delta.y * controller->controllerSensitivity;  // The y-axis controls the pitch
                rotation.y -= delta.x * controller->controllerSensitivity;  // The x-axis controls the yaw
            }

            // We prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
            if (rotation.x < -glm::half_pi<float>() * 0.99f) rotation.x = -glm::half_pi<float>() * 0.99f;
            if (rotation.x > glm::half_pi<float>() * 0.99f) rotation.x = glm::half_pi<float>() * 0.99f;
            // This is not necessary, but whenever the rotation goes outside the 0 to 2*PI range, we wrap it back
            // inside. This could prevent floating point error if the player rotates in single direction for an
            // extremely long time.
            rotation.y = glm::wrapAngle(rotation.y);

            if (baseFov <= 0.0f) baseFov = camera->fovY;

            aiming = app->getMouse().isPressed(GLFW_MOUSE_BUTTON_RIGHT);
            aiming |= app->getJoystick().isTriggerPressed(0);  // also check joystick input

            currentAimSpeed = controller->aimSpeed;
            float targetFov;
            if (aiming) {
                targetFov = controller->aimFovY;
                controller->rotationSensitivity =
                    controller->baseRotationSensitivity * controller->aimSensitivityMultiplier;
                controller->controllerSensitivity =
                    controller->controllerBaseSensitivity * controller->aimSensitivityMultiplier;
            } else {
                targetFov = baseFov;
                controller->rotationSensitivity = controller->baseRotationSensitivity;
                controller->controllerSensitivity = controller->controllerBaseSensitivity;
            }
            camera->fovY = glm::mix(camera->fovY, targetFov, glm::clamp(controller->aimSpeed * deltaTime, 0.0f, 1.0f));
        }

        bool isAiming() const {
            return aiming;
        }

        float getAimSpeed() const {
            return currentAimSpeed;
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit() {
            if (mouse_locked) {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
                app->getMouse().enable(app->getWindow());
            }
        }
    };

}  // namespace our
