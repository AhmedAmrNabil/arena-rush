#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    enum class ColliderShape { Sphere, Capsule };

    class ColliderComponent : public our::Component {
    public:
        ColliderShape shape = ColliderShape::Sphere;
        std::string layer = "default";
        float radius = 0.5f;
        float height = 1.0f;  // must be the total height
        bool isTrigger = false;

        static std::string getID() {
            return "Collider";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
