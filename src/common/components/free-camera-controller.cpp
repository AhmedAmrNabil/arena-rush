#include "free-camera-controller.hpp"

#include <glm/gtc/constants.hpp>

namespace our {
    // Reads sensitivities & speedupFactor from the given json object
    void FreeCameraControllerComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        baseRotationSensitivity = data.value("baseRotationSensitivity", baseRotationSensitivity);
        aimSensitivityMultiplier = data.value("aimSensitivityMultiplier", aimSensitivityMultiplier);
        aimFovY = data.value("aimFovY", 30.0f) * (glm::pi<float>() / 180.0f);
        aimSpeed = data.value("aimSpeed", aimSpeed);
    }
}  // namespace our
