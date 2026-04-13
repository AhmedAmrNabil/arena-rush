#pragma once
#include <glm/glm.hpp>
#include <json/json.hpp>

#include "../ecs/component.hpp"

namespace our {

    enum class LightType { DIRECTIONAL, POINT, SPOT };

    class Light : public Component {
    public:
        LightType type;
        glm::vec3 color;
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 attenuation;  // (constant, linear, quadratic)
        glm::vec2 spotAngles;   // (inner, outer) in degrees

        void deserialize(const nlohmann::json& data);
        static std::string getID() {
            return "Light";
        }
    };
}  // namespace our
