#pragma once

#include <components/collider.hpp>
#include <ecs/entity.hpp>
#include <ecs/world.hpp>
#include <glm/glm.hpp>

// Forward declaration. So that we don't include Bullet headers here, instead include in .cpp file to reduce compilation
// time for files that include this header
class btCollisionWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btCollisionObject;
class btCollisionShape;

namespace gameplay {

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;  // normalized
    };

    struct HitInfo {
        bool hit = false;               // whether the ray hit anything
        float distance = 0.0f;          // the distance from the ray origin to the point of intersection
        glm::vec3 point;                // the point of intersection in world space
        glm::vec3 normal;               // the normal at the point of intersection in world space
        our::Entity* entity = nullptr;  // the entity that was hit
    };

    struct CollisionEvent {
        // The two entities that overlapped
        our::Entity* entityA = nullptr;
        our::Entity* entityB = nullptr;

        glm::vec3 point;
        glm::vec3 normal;               // from A to B
        float penetrationDepth = 0.0f;  // how much the two colliders are penetrating each other
    };

    enum CollisionLayer : short {
        LAYER_PLAYER = 1 << 0,       // bit 0:  0000 0001
        LAYER_ENEMY = 1 << 1,        // bit 1:  0000 0010
        LAYER_ENVIRONMENT = 1 << 2,  // bit 2:  0000 0100
        LAYER_PROJECTILE = 1 << 3,   // bit 3:  0000 1000
        LAYER_TRIGGER = 1 << 4,      // bit 4:  0001 0000
    };

    short layerStringToGroup(const std::string& layer);
    short getMaskForLayer(short group);

    class CollisionSystem {
    private:
        // Bullet internals
        btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
        btCollisionDispatcher* dispatcher = nullptr;
        btDbvtBroadphase* broadphase = nullptr;
        btCollisionWorld* collisionWorld = nullptr;

        // Entity-Bullet mappings
        std::unordered_map<our::Entity*, btCollisionObject*> entityToBullet;
        std::vector<btCollisionShape*> ownedShapes;

        // Frame results
        std::vector<CollisionEvent> frameCollisions;

        // Internal helper functions
        void addEntity(our::Entity* entity);
        void removeEntity(our::Entity* entity);
        void syncTransform(our::Entity* entity);

    public:
        void initialize();
        void destroy();

        void update(our::World* world);

        // On-demand raycast function that can be used outside of the update loop to query the world for collisions.
        HitInfo raycast(const Ray& ray, float maxDistance, const std::string targetLayer = "") const;

        // On-demand overlap sphere function that can be used outside of the update loop
        std::vector<our::Entity*> overlapSphere(const glm::vec3& center, float radius, std::string targetLayer = "");

        const std::vector<CollisionEvent>& getCollisions() const;
    };

}  // namespace gameplay
