#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class WeaponComponent : public our::Component {
    public:
        int damage;
        float range;
        float fireRate;
        int maxAmmo;
        float reloadTime;
        int currentAmmo;
        static std::string getID() {
            return "Weapon";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
