#pragma once
#include <glm/glm.hpp>
#include <json/json.hpp>

#include "../ecs/component.hpp"

#define MAX_LIGHTS 24  // we can increase later, but don't forget to update in frag shader too
namespace our {

    enum class LightType { DIRECTIONAL, POINT, SPOT };

    struct LightRenderData {
        LightType type;
        glm::vec3 color;
        glm::vec3 attenuation;  // (constant, linear, quadratic)
        glm::vec2 spotAngles;   // (inner, outer) in radians

        // computed at render time in forward renderer
        glm::vec3 position;   // world space position of the light (for point and spot lights)
        glm::vec3 direction;  // world space direction of the light (for directional and spot lights)
    };

    class Light : public Component {
    public:
        LightType type;
        glm::vec3 color;
        // removed position and direction
        // as they are controlled by entity's transform component
        glm::vec3 attenuation;  // (constant, linear, quadratic)
        glm::vec2 spotAngles;   // (inner, outer) in radians

        void deserialize(const nlohmann::json& data);
        static std::string getID() {
            return "light";
        }
    };
}  // namespace our
