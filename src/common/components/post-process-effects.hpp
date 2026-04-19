#pragma once

#include <ecs/component.hpp>
#include <string>
#include <unordered_map>

namespace our {

    // values set by gameplay and read by the renderer for the active postprocess shader
    class PostProcessEffectsComponent : public Component {
    public:
        std::unordered_map<std::string, float> uniforms;

        static std::string getID() {
            return "Post Process Effects Component";
        }

        void deserialize(const nlohmann::json& data) override {}
    };

}  // namespace our
