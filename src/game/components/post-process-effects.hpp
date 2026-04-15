#pragma once

#include <ecs/component.hpp>
#include <string>
#include <unordered_map>

namespace gameplay {

    // values set in gameplay and renderer sets them for the current postprocess shader
    // (if we need more than one process shader, the renderer needs a way to know which shader the values are for)
    class PostProcessEffectsComponent : public our::Component {
    public:
        std::unordered_map<std::string, float> uniforms;

        static std::string getID() {
            return "Post Process Effects Component";
        }

        void deserialize(const nlohmann::json& data) override {}
    };

}  // namespace gameplay
