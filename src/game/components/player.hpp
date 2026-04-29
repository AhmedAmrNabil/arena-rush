#pragma once

#include <components/weapon.hpp>
#include <ecs/component.hpp>

namespace gameplay {

    class PlayerComponent : public our::Component {
    public:
        int currentActiveWeaponIndex = 0;
        gameplay::WeaponComponent* currentWeapon = nullptr;
        static std::string getID() {
            return "Player";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
