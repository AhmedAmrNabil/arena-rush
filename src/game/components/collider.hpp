#pragma once

#include <ecs/component.hpp>
#include <glm/glm.hpp>
#include <model/model.hpp>

struct btTriangleMesh;
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
        std::string shapeCacheId;  // explicit key to reuse Bullet shapes across similar entities
        CollisionLayer layer = CollisionLayer::LAYER_ENVIRONMENT;
        float radius = 0.5f;
        float height = 1.0f;  // must be the total height
        bool isTrigger = false;
        bool worldSpace = false;  // if true, radius/height are in world units (skip entity scale)
        glm::vec3 centerOffset =
            glm::vec3(0.0f);        // local offset applied to the Bullet transform (e.g. shift capsule up)
        our::Mesh* mesh = nullptr;  // optional mesh for mesh colliders

        static std::string getID() {
            return "Collider";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace gameplay
