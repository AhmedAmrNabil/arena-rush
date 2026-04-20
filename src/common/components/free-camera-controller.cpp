#include "free-camera-controller.hpp"

namespace our {
    // Reads sensitivities & speedupFactor from the given json object
    void FreeCameraControllerComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        rotationSensitivity = data.value("rotationSensitivity", rotationSensitivity);
        fovSensitivity = data.value("fovSensitivity", fovSensitivity);
    }
}  // namespace our
