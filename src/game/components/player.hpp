#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class PlayerComponent : public our::Component {
    public:
        int currentAmmo = 30;
        int magSize = 30;
        int maxAmmo = 120;
        static std::string getID() {
            return "Player";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
