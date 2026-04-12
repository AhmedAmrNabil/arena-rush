#include "camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../ecs/entity.hpp"

namespace our {
    // Reads camera parameters from the given json object
    void CameraComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        std::string cameraTypeStr = data.value("cameraType", "perspective");
        if (cameraTypeStr == "orthographic") {
            cameraType = CameraType::ORTHOGRAPHIC;
        } else {
            cameraType = CameraType::PERSPECTIVE;
        }
        near = data.value("near", 0.01f);
        far = data.value("far", 100.0f);
        fovY = data.value("fovY", 90.0f) * (glm::pi<float>() / 180);
        orthoHeight = data.value("orthoHeight", 1.0f);
    }

    // Creates and returns the camera view matrix
    glm::mat4 CameraComponent::getViewMatrix() const {
        auto owner = getOwner();
        auto M = owner->getLocalToWorldMatrix();
        // first transform the camera to use the owner entity's local to world matrix,
        // then use glm::lookAt to create the view matrix
        glm::vec3 eye = glm::vec3(M * glm::vec4(0, 0, 0, 1));
        glm::vec3 center = glm::vec3(M * glm::vec4(0, 0, -1, 1));
        glm::vec3 up = glm::vec3(M * glm::vec4(0, 1, 0, 0));
        return glm::lookAt(eye, center, up);
    }

    // Creates and returns the camera projection matrix
    // "viewportSize" is used to compute the aspect ratio
    glm::mat4 CameraComponent::getProjectionMatrix(glm::ivec2 viewportSize) const {
        float aspectRatio = static_cast<float>(viewportSize.x) / viewportSize.y;
        if (cameraType == CameraType::ORTHOGRAPHIC) {
            float orthoWidth = orthoHeight * aspectRatio;
            return glm::ortho(-orthoWidth / 2, orthoWidth / 2, -orthoHeight / 2, orthoHeight / 2, near, far);
        } else {
            return glm::perspective(fovY, aspectRatio, near, far);
        }
    }
}  // namespace our
