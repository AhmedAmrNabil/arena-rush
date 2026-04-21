#pragma once

#include <glm/glm.hpp>

#include "../ecs/component.hpp"

namespace our {

    // This component denotes that the FreeCameraControllerSystem will move the owning entity using user inputs.
    // This component is added as a slightly complex example for how use the ECS framework to implement logic.
    // For more information, see "common/systems/free-camera-controller.hpp"
    // For a more simple example of how to use the ECS framework, see "movement.hpp"
    class FreeCameraControllerComponent : public Component {
    public:
        // The senstivity paramter defined sensitive the camera rotation & fov is to the mouse moves and wheel scrolling
        float rotationSensitivity = 0.01f;  // The angle change per pixel of mouse movement

        // Aim-down-sight
        float aimFovY = glm::radians(30.0f);
        float aimSpeed = 8.0f;

        // The ID of this component type is "Free Camera Controller"
        static std::string getID() {
            return "Free Camera Controller";
        }

        // Reads sensitivities & speedupFactor from the given json object
        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace our
