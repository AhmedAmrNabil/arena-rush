#pragma once

#include <components/collider.hpp>
#include <cstdint>
#include <ecs/entity.hpp>
#include <ecs/world.hpp>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "collision-debug-drawer.hpp"

// Forward declaration. So that we don't include Bullet headers here, instead include in .cpp file to reduce compilation
// time for files that include this header
class btCollisionWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btCollisionObject;
class btCollisionShape;
class btTriangleMesh;

#ifdef COLLISION_DEBUG_DRAW
class CollisionDebugDrawer;
#endif

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
        glm::vec3 normal;               // world-space normal on entity B
        float penetrationDepth = 0.0f;  // how much the two colliders are penetrating each other
    };

    short getMaskForLayer(short group);

    class CollisionSystem {
    private:
        // Bullet internals
        btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
        btCollisionDispatcher* dispatcher = nullptr;
        btDbvtBroadphase* broadphase = nullptr;
        btCollisionWorld* collisionWorld = nullptr;

#ifdef COLLISION_DEBUG_DRAW
        CollisionDebugDrawer* debugDrawer = nullptr;
        bool debugDrawEnabled = false;
#endif

        // Entity-Bullet mappings
        std::unordered_map<our::Entity*, btCollisionObject*> entityToBullet;
        std::unordered_map<btCollisionShape*, std::uint16_t> ownedShapes;
        std::unordered_map<std::string, btCollisionShape*> shapesCache;
        std::unordered_map<std::string, btTriangleMesh*> meshDataCache;

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
        HitInfo raycast(const Ray& ray, float maxDistance,
                        const short targetLayer = CollisionLayer::LAYER_ENVIRONMENT) const;

        // On-demand overlap sphere function that can be used outside of the update loop
        std::vector<our::Entity*> overlapSphere(const glm::vec3& center, float radius, short targetLayer);

        const std::vector<CollisionEvent>& getCollisions() const;

#ifdef COLLISION_DEBUG_DRAW
        /// Call after the main renderer finishes to overlay collision wireframes.
        /// @param VP  the View-Projection matrix from the active camera.
        void debugDraw(const glm::mat4& VP);
        void setDebugDrawEnabled(bool enabled);
        bool isDebugDrawEnabled() const;
#endif
    };

}  // namespace gameplay
