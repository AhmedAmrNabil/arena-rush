#include "collision-system.hpp"

#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/Gimpact/btGImpactShape.h>
#include <btBulletCollisionCommon.h>

#include <components/collider.hpp>
#include <ecs/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/player.hpp"

#ifdef COLLISION_DEBUG_DRAW
#include "collision-debug-drawer.hpp"
#endif

// Helper functions for GLM-Bullet conversions
static inline btVector3 glmToBtVec3(const glm::vec3& v) {
    return btVector3(v.x, v.y, v.z);
}
static inline glm::vec3 btToGlmVec3(const btVector3& v) {
    return glm::vec3(v.getX(), v.getY(), v.getZ());
}
static inline glm::vec3 worldToLocal(const our::Entity* entity, glm::vec3 worldVector) {
    if (entity->parent) {
        glm::mat4 parentWorld = entity->parent->getLocalToWorldMatrix();
        glm::mat3 rotationMatrix = glm::mat3(parentWorld);  // extract the 3x3 rotation part
        glm::mat3 inverseRotation = glm::transpose(rotationMatrix);
        return inverseRotation * worldVector;  // apply inverse rotation to get local vector
    }

    return worldVector;  // if no parent then local and world are the same
}

static btTransform entityToBtTransform(our::Entity* entity) {
    glm::mat4 m = entity->getLocalToWorldMatrix();

    // Extract position from column 3
    glm::vec3 pos = glm::vec3(m[3]);

    // Extract rotation by stripping scale from the 3x3 submatrix.
    // Each column of the 3x3 has length = scale along that axis.
    // Normalizing each column removes the scale, leaving pure rotation.
    glm::mat3 rotMat;
    rotMat[0] = glm::normalize(glm::vec3(m[0]));
    rotMat[1] = glm::normalize(glm::vec3(m[1]));
    rotMat[2] = glm::normalize(glm::vec3(m[2]));
    glm::quat rot = glm::quat_cast(rotMat);

    btTransform t;
    t.setOrigin(btVector3(pos.x, pos.y, pos.z));
    t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
    return t;
}

static std::string makeShapeScaleSuffix(const glm::vec3& scale) {
    std::string out;
    out = "_Scale_" + std::to_string(scale.x) + "_" + std::to_string(scale.y) + "_" + std::to_string(scale.z);
    return out;
}

static btTransform entityToBtTransformYawOnly(our::Entity* entity) {
    glm::mat4 m = entity->getLocalToWorldMatrix();
    glm::vec3 pos = glm::vec3(m[3]);

    // Extract yaw from the forward vector projected onto XZ plane
    glm::vec3 forward = glm::normalize(glm::vec3(m[2]));  // local -Z in world space
    float yaw = atan2(forward.x, forward.z);              // yaw angle from forward direction

    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(pos.x, pos.y, pos.z));
    t.setRotation(btQuaternion(btVector3(0, 1, 0), yaw));  // only rotate around Y
    return t;
}

namespace gameplay {

    inline short getMaskForLayer(short group) {
        switch (group) {
            case LAYER_PLAYER:
                return LAYER_ENEMY | LAYER_ENVIRONMENT | LAYER_TRIGGER | LAYER_PROJECTILE;
            case LAYER_ENEMY:
                return LAYER_PLAYER | LAYER_PROJECTILE | LAYER_ENVIRONMENT;
            case LAYER_ENVIRONMENT:
                return LAYER_PLAYER | LAYER_ENEMY | LAYER_PROJECTILE;
            case LAYER_PROJECTILE:
                return LAYER_PLAYER | LAYER_ENEMY | LAYER_ENVIRONMENT;
            case LAYER_TRIGGER:
                return LAYER_PLAYER;
            default:
                return 0;
        }
    }

    void CollisionSystem::initialize() {
        // collision configuration contains default setup for memory, collision setup.
        collisionConfiguration = new btDefaultCollisionConfiguration();

        // use the default collision dispatcher
        dispatcher = new btCollisionDispatcher(collisionConfiguration);

        broadphase = new btDbvtBroadphase();

        // the default constraint solver
        collisionWorld = new btCollisionWorld(dispatcher, broadphase, collisionConfiguration);

        // register GImpact algorithm for mesh collisions
        btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);

#ifdef COLLISION_DEBUG_DRAW
        debugDrawer = new CollisionDebugDrawer();
        debugDrawer->initialize();
        debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
        collisionWorld->setDebugDrawer(debugDrawer);
#endif
    }

    void CollisionSystem::destroy() {
#ifdef COLLISION_DEBUG_DRAW
        if (debugDrawer) {
            debugDrawer->destroy();
            delete debugDrawer;
            debugDrawer = nullptr;
        }
#endif

        // remove collision objects
        for (auto& [entity, obj] : entityToBullet) {
            collisionWorld->removeCollisionObject(obj);
            delete obj;
        }
        entityToBullet.clear();

        // remove shapes
        for (auto [shape, value] : ownedShapes) {
            delete shape;
        }
        ownedShapes.clear();
        shapesCache.clear();

        for (auto& [key, mesh] : meshDataCache) {
            delete mesh;
        }
        meshDataCache.clear();

        // delete bullet internals
        delete collisionWorld;
        delete broadphase;
        delete dispatcher;
        delete collisionConfiguration;
        collisionWorld = nullptr;
        broadphase = nullptr;
        dispatcher = nullptr;
        collisionConfiguration = nullptr;

        frameCollisions.clear();
    }

    void CollisionSystem::handleCollisions(const CollisionEvent& event) {
        auto* colliderA = event.entityA->getComponent<ColliderComponent>();
        auto* colliderB = event.entityB->getComponent<ColliderComponent>();

        if (!colliderA || !colliderB) return;
        if (colliderA->isTrigger || colliderB->isTrigger) return;

        // clang-format off
        // push back logic (don't push environments)
        const float SKIN_WIDTH = 0.01f; // a small extra distance to prevent sticking
        if (colliderA->layer == CollisionLayer::LAYER_ENVIRONMENT) {
            event.entityB->localTransform.position -= worldToLocal(event.entityB, event.normal * (event.penetrationDepth + SKIN_WIDTH));
        } else if (colliderB->layer == CollisionLayer::LAYER_ENVIRONMENT) {
            event.entityA->localTransform.position += worldToLocal(event.entityA, event.normal * (event.penetrationDepth + SKIN_WIDTH));
        } else {
            event.entityA->localTransform.position += worldToLocal(event.entityA, event.normal * ((event.penetrationDepth + SKIN_WIDTH) / 2.0f));
            event.entityB->localTransform.position -= worldToLocal(event.entityB, event.normal * ((event.penetrationDepth + SKIN_WIDTH) / 2.0f));
        }
        // clang-format on
    }

    void CollisionSystem::update(our::World* world) {
        // clear previous frame's collisions
        frameCollisions.clear();

        // Sync entities with colliders to Bullet
        for (auto entity : world->getEntities()) {
            ColliderComponent* collider = entity->getComponent<ColliderComponent>();

            if (!collider) continue;

            if (entityToBullet.find(entity) == entityToBullet.end()) {
                addEntity(entity);
            } else {
                syncTransform(entity);
            }
        }

        // Remove bullet objects for entities that no longer exists
        std::vector<our::Entity*> toRemove;
        for (const auto& [entity, obj] : entityToBullet) {
            if (world->getEntities().find(entity) == world->getEntities().end()) {
                toRemove.push_back(entity);
            }
        }
        for (our::Entity* entity : toRemove) {
            removeEntity(entity);
        }

        // Run collision detection
        collisionWorld->performDiscreteCollisionDetection();

        // Read collision events
        int numManifolds = dispatcher->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            // any potential collision will have a manifold, even if it has no contact points
            btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(i);
            const btCollisionObject* objA = manifold->getBody0();
            const btCollisionObject* objB = manifold->getBody1();
            our::Entity* entityA = static_cast<our::Entity*>(objA->getUserPointer());
            our::Entity* entityB = static_cast<our::Entity*>(objB->getUserPointer());

            // check if they collide (if they have contact points)
            // find deepest contact point (the one with the largest penetration depth)
            int numContacts = manifold->getNumContacts();
            if (numContacts == 0) continue;
            float deepest = 0.0f;
            int deepestIndex = -1;
            for (int j = 0; j < numContacts; j++) {
                // negative distance means penetration, and positive distance means separation. So we want the most
                // negative distance.
                float d = manifold->getContactPoint(j).getDistance();
                if (d < deepest) {
                    deepest = d;
                    deepestIndex = j;
                }
            }

            if (deepestIndex != -1) {
                // we have a collision!
                btManifoldPoint& contactPoint = manifold->getContactPoint(deepestIndex);
                CollisionEvent event;
                event.entityA = entityA;
                event.entityB = entityB;
                event.point = btToGlmVec3(contactPoint.getPositionWorldOnB());
                event.normal = btToGlmVec3(contactPoint.m_normalWorldOnB);
                event.penetrationDepth = -contactPoint.getDistance();  // convert back to positive penetration depth
                frameCollisions.push_back(event);
            }
        }

        for (const CollisionEvent& event : frameCollisions) {
            handleCollisions(event);
        }
    }

    // On-demand function
    HitInfo CollisionSystem::raycast(const Ray& ray, float maxDistance, const short targetLayer) const {
        HitInfo hitInfo;

        if (!collisionWorld) return hitInfo;
        btVector3 from = glmToBtVec3(ray.origin);
        btVector3 to = glmToBtVec3(ray.origin + ray.direction * maxDistance);

        btCollisionWorld::ClosestRayResultCallback callback(from, to);

        callback.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;  // check against all layers
        callback.m_collisionFilterMask = targetLayer;                    // only collide with the target layer

        collisionWorld->rayTest(from, to, callback);

        if (callback.hasHit()) {
            hitInfo.hit = true;
            hitInfo.entity = static_cast<our::Entity*>(callback.m_collisionObject->getUserPointer());
            hitInfo.point = btToGlmVec3(callback.m_hitPointWorld);
            hitInfo.normal = btToGlmVec3(callback.m_hitNormalWorld);
            hitInfo.distance = callback.m_closestHitFraction * maxDistance;
        }

        return hitInfo;
    }

    // On-demand function
    std::vector<our::Entity*> CollisionSystem::overlapSphere(const glm::vec3& center, float radius, short targetLayer) {
        std::vector<our::Entity*> results;
        if (!collisionWorld) return results;

        for (const auto& [entity, obj] : entityToBullet) {
            short objGroup = obj->getBroadphaseHandle()->m_collisionFilterGroup;
            if ((objGroup & targetLayer) == 0) continue;

            glm::vec3 entityPos = btToGlmVec3(obj->getWorldTransform().getOrigin());

            ColliderComponent* collider = entity->getComponent<ColliderComponent>();
            float entityRadius = collider ? collider->radius : 0.0f;

            // sphere vs sphere hit check
            float dist = glm::length(entityPos - center);
            if (dist <= radius + entityRadius) {
                results.push_back(entity);
            }
        }

        return results;
    }

    Aabb CollisionSystem::getWorldAabb(const our::Entity* entity) const {
        Aabb bounds;
        if (!entity) return bounds;

        auto it = entityToBullet.find(const_cast<our::Entity*>(entity));
        if (it == entityToBullet.end()) return bounds;

        btCollisionObject* obj = it->second;
        if (!(obj && obj->getCollisionShape())) return bounds;

        btVector3 minPoint;
        btVector3 maxPoint;
        obj->getCollisionShape()->getAabb(entityToBtTransform(const_cast<our::Entity*>(entity)), minPoint, maxPoint);

        bounds.min = btToGlmVec3(minPoint);
        bounds.max = btToGlmVec3(maxPoint);
        bounds.valid = true;
        return bounds;
    }

    void CollisionSystem::addEntity(our::Entity* entity) {
        auto* collider = entity->getComponent<ColliderComponent>();
        if (!collider) return;

        // Create or reuse collision shape based on collider data
        btCollisionShape* shape = nullptr;

        std::string shapeKey;
        const std::string scaleSuffix =
            collider->worldSpace ? "_WS" : makeShapeScaleSuffix(entity->localTransform.scale);
        if (!collider->shapeCacheId.empty()) {
            shapeKey = std::string("ID_") + collider->shapeCacheId + scaleSuffix;
        } else {
            shapeKey = std::to_string(static_cast<int>(collider->shape)) + "_" + std::to_string(collider->radius) +
                       "_" + std::to_string(collider->height) + scaleSuffix;
        }

        if (shapesCache.find(shapeKey) != shapesCache.end()) {
            ownedShapes[shapesCache[shapeKey]]++;
            shape = shapesCache[shapeKey];
        } else {
            // Create new shape and cache it
            switch (collider->shape) {
                case ColliderShape::Sphere:
                    shape = new btSphereShape(collider->radius);
                    break;
                case ColliderShape::Capsule: {
                    float spine = collider->height - 2.0f * collider->radius;
                    if (spine < 0.0f) spine = 0.0f;
                    shape = new btCapsuleShape(collider->radius, spine);
                    break;
                }
                case ColliderShape::Mesh:
                    if (collider->mesh) {
                        btTriangleMesh* triMesh = new btTriangleMesh();
                        const std::vector<our::Vertex>& vertices = collider->mesh->getVertices();
                        const std::vector<unsigned int>& indices = collider->mesh->getIndices();
                        triMesh->preallocateVertices(vertices.size());
                        triMesh->preallocateIndices(indices.size());
                        for (size_t i = 0; i < indices.size(); i += 3) {
                            const glm::vec3& v0 = vertices[indices[i]].position;
                            const glm::vec3& v1 = vertices[indices[i + 1]].position;
                            const glm::vec3& v2 = vertices[indices[i + 2]].position;
                            triMesh->addTriangle(glmToBtVec3(v0), glmToBtVec3(v1), glmToBtVec3(v2));
                        }
                        shape = new btBvhTriangleMeshShape(triMesh, true);
                        meshDataCache[shapeKey] = triMesh;
                    } else {
                        std::cerr << "[Collision] Collider mesh is null for entity " << entity->name
                                  << ". Defaulting to sphere shape." << std::endl;
                        shape = new btSphereShape(collider->radius);
                    }
                    break;
            }
            if (!collider->worldSpace) {
                shape->setLocalScaling(glmToBtVec3(entity->localTransform.scale));
            }
            shapesCache[shapeKey] = shape;
            ownedShapes[shape] = 1;
        }

        // Create the collision object
        btCollisionObject* obj = new btCollisionObject();
        obj->setCollisionShape(shape);

        auto* player = entity->getComponent<PlayerComponent>();
        btTransform transform;
        if (player) {
            transform = entityToBtTransformYawOnly(entity);
        } else {
            transform = entityToBtTransform(entity);
        }

        // Apply center offset (e.g. shift capsule up so bottom aligns with model feet)
        if (collider->centerOffset != glm::vec3(0.0f)) {
            btVector3 offset = glmToBtVec3(collider->centerOffset);
            transform.setOrigin(transform.getOrigin() + offset);
        }

        obj->setWorldTransform(transform);

        obj->setUserPointer(entity);  // so we can go back to the entity

        if (collider->layer == CollisionLayer::LAYER_ENVIRONMENT) {
            // Walls and floors remain strictly static
            obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
        } else {
            // Players, Moons, and moving hazards are "Dynamic" (Flags = 0)
            obj->setCollisionFlags(0);

            // Prevent the object from going to sleep!
            obj->setActivationState(DISABLE_DEACTIVATION);
        }

        // Add to Bullet world with layer filtering
        short group = collider->layer;
        short mask = getMaskForLayer(group);
        collisionWorld->addCollisionObject(obj, group, mask);
        entityToBullet[entity] = obj;
    }

    void CollisionSystem::removeEntity(our::Entity* entity) {
        if (entityToBullet.find(entity) == entityToBullet.end()) return;

        btCollisionObject* obj = entityToBullet[entity];
        btCollisionShape* shape = obj->getCollisionShape();

        if (shape) {
            ownedShapes[shape]--;
            if (ownedShapes[shape] == 0) {
                ownedShapes.erase(shape);

                // Erase it from the cache
                for (auto it = shapesCache.begin(); it != shapesCache.end(); ++it) {
                    if (it->second == shape) {
                        auto meshIt = meshDataCache.find(it->first);
                        if (meshIt != meshDataCache.end()) {
                            delete meshIt->second;
                            meshDataCache.erase(meshIt);
                        }

                        shapesCache.erase(it);
                        break;
                    }
                }

                delete shape;
            }
        }

        collisionWorld->removeCollisionObject(obj);
        delete obj;
        entityToBullet.erase(entity);
    }

    void CollisionSystem::syncTransform(our::Entity* entity) {
        btCollisionObject* obj = entityToBullet[entity];

        if (!obj) return;

        auto* player = entity->getComponent<PlayerComponent>();
        auto* collider = entity->getComponent<ColliderComponent>();

        btTransform transform;
        if (player) {
            // for players, we only update the yaw rotation to prevent them from tipping over when walking on slopes or
            // getting hit by hazards. This is a common technique in character controllers to improve stability.
            transform = entityToBtTransformYawOnly(entity);
        } else {
            transform = entityToBtTransform(entity);
        }

        // Apply center offset (must match what was done in addEntity)
        if (collider && collider->centerOffset != glm::vec3(0.0f)) {
            btVector3 offset = glmToBtVec3(collider->centerOffset);
            transform.setOrigin(transform.getOrigin() + offset);
        }

        obj->setWorldTransform(transform);

        // Update the broadphase AABB so Bullet uses the new position for collision detection
        collisionWorld->updateSingleAabb(obj);
    }
    const std::vector<CollisionEvent>& CollisionSystem::getCollisions() const {
        return frameCollisions;
    }

#ifdef COLLISION_DEBUG_DRAW
    // Returns a debug color based on the collision layer of the entity
    static glm::vec3 getLayerColor(our::Entity* entity) {
        auto* collider = entity->getComponent<ColliderComponent>();
        if (!collider) return glm::vec3(1, 1, 1);

        switch (collider->layer) {
            case LAYER_PLAYER:
                return glm::vec3(0.0f, 1.0f, 0.0f);  // green
            case LAYER_ENEMY:
                return glm::vec3(1.0f, 0.2f, 0.2f);  // red
            case LAYER_ENVIRONMENT:
                return glm::vec3(0.0f, 0.8f, 1.0f);  // cyan
            case LAYER_PROJECTILE:
                return glm::vec3(1.0f, 1.0f, 0.0f);  // yellow
            case LAYER_TRIGGER:
                return glm::vec3(1.0f, 0.0f, 1.0f);  // magenta
            default:
                return glm::vec3(1.0f, 1.0f, 1.0f);  // white
        }
    }

    void CollisionSystem::debugDraw(const glm::mat4& VP) {
        if (!debugDrawEnabled || !collisionWorld || !debugDrawer) return;

        // Manually generate wireframe geometry for every collision object.
        // We do NOT use Bullet's debugDrawObject/debugDrawWorld because btCollisionWorld
        // (as opposed to btDynamicsWorld) has very limited debug draw support.
        for (const auto& [entity, obj] : entityToBullet) {
            auto* collider = entity->getComponent<ColliderComponent>();
            if (!collider) continue;

            glm::vec3 color = getLayerColor(entity);

            // Build a glm::mat4 from the Bullet world transform
            btTransform bt = obj->getWorldTransform();
            float m[16];
            bt.getOpenGLMatrix(m);
            glm::mat4 transform = glm::make_mat4(m);

            // Apply the collision shape's local scaling
            btVector3 scale = obj->getCollisionShape()->getLocalScaling();
            float scaledRadius = collider->radius * scale.getX();
            float scaledHeight = collider->height * scale.getY();
            transform = transform * glm::scale(glm::mat4(1.0f), glm::vec3(scale.getX(), scale.getY(), scale.getZ()));

            switch (collider->shape) {
                case ColliderShape::Sphere:
                    debugDrawer->drawSphereWireframe(transform, scaledRadius, color);
                    break;
                case ColliderShape::Capsule:
                    debugDrawer->drawCapsuleWireframe(transform, scaledRadius, scaledHeight, color);
                    break;
                case ColliderShape::Mesh:
                    if (collider->mesh) {
                        debugDrawer->drawMeshWireframe(collider->mesh, transform, color);
                    }
                    break;
            }
        }

        // Flush all accumulated lines to the screen
        debugDrawer->render(VP);
    }

    void CollisionSystem::setDebugDrawEnabled(bool enabled) {
        debugDrawEnabled = enabled;
    }

    bool CollisionSystem::isDebugDrawEnabled() const {
        return debugDrawEnabled;
    }
#endif

}  // namespace gameplay
