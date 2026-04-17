#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class PlayerComponent : public our::Component {
    public:
        static std::string getID() {
            return "Player";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
