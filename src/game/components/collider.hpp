#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    enum class ColliderShape { Sphere, Capsule, Mesh };

    enum CollisionLayer : short {
        LAYER_PLAYER = 1 << 0,       // bit 0:  0000 0001
        LAYER_ENEMY = 1 << 1,        // bit 1:  0000 0010
        LAYER_ENVIRONMENT = 1 << 2,  // bit 2:  0000 0100
        LAYER_PROJECTILE = 1 << 3,   // bit 3:  0000 1000
        LAYER_TRIGGER = 1 << 4,      // bit 4:  0001 0000
    };

    class ColliderComponent : public our::Component {
    public:
        ColliderShape shape = ColliderShape::Sphere;
        CollisionLayer layer = CollisionLayer::LAYER_ENVIRONMENT;
        float radius = 0.5f;
        float height = 1.0f;  // must be the total height
        bool isTrigger = false;

        std::string meshModelName;

        static std::string getID() {
            return "Collider";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
