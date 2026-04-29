#pragma once

#include <ecs/component.hpp>
#include <string>

namespace gameplay {

    class ProjectileTrailComponent : public our::Component {
    public:
        std::string material;
        std::string mesh = "sphere";
        int maxSegments = 8;
        float segmentLifetime = 0.12f;
        float headScale = 0.12f;   // scale at birth (wider/brighter near bullet)
        float tailScale = 0.015f;  // scale at death (thinner/dimmer at tail)

        float spawnTimer = 0.0f;

        float segmentInterval() const {
            return maxSegments > 0 ? (segmentLifetime / static_cast<float>(maxSegments)) : segmentLifetime;
        }

        static std::string getID() {
            return "Projectile Trail";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
