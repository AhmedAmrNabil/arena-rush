#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class PlayerComponent : public our::Component {
    public:
        float radius = 0.5f;
        float height = 1.8f;

        static std::string getID() {
            return "Player";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
