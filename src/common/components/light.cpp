#include "light.hpp"

#include <deserialize-utils.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <json/json.hpp>

namespace our {

    void Light::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        std::string typeStr = data.value("lightType", "point");
        if (typeStr == "directional") {
            type = LightType::DIRECTIONAL;
        } else if (typeStr == "point") {
            type = LightType::POINT;
        } else if (typeStr == "spot") {
            type = LightType::SPOT;
        }
        color = data.value("color", glm::vec3(1.0f, 1.0f, 1.0f));
        attenuation = data.value("attenuation", glm::vec3(1.0f, 0.0f, 0.0f));
        spotAngles = data.value("spotAngles", glm::vec2(15.0f, 30.0f));
    }
}  // namespace our
