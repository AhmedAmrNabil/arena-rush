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
        glm::vec4 eye = M * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 center = M * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
        glm::vec4 up = M * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        return glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(up));
    }

    // Creates and returns the camera projection matrix
    // "viewportSize" is used to compute the aspect ratio
    glm::mat4 CameraComponent::getProjectionMatrix(glm::ivec2 viewportSize) const {
        if (viewportSize.x == 0 || viewportSize.y == 0) return glm::mat4(1.0f);

        float aspectRatio = (float)viewportSize.x / (float)viewportSize.y;

        if (cameraType == CameraType::ORTHOGRAPHIC) {
            float orthoWidth = orthoHeight * aspectRatio;
            return glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoHeight / 2.0f, orthoHeight / 2.0f, near,
                              far);
        } else {
            return glm::perspective(fovY, aspectRatio, near, far);
        }
    }
}  // namespace our
