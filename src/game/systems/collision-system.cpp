#include "collision-system.hpp"

#include <btBulletCollisionCommon.h>

#include <asset-loader.hpp>
#include <components/collider.hpp>
#include <components/model-renderer.hpp>
#include <ecs/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <mesh/mesh.hpp>
#include <mesh/vertex.hpp>
#include <model/model.hpp>

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

namespace gameplay {

    inline short getMaskForLayer(short group) {
        switch (group) {
            case LAYER_PLAYER:
                return LAYER_ENEMY | LAYER_ENVIRONMENT | LAYER_TRIGGER;
            case LAYER_ENEMY:
                return LAYER_PLAYER | LAYER_PROJECTILE | LAYER_ENVIRONMENT;
            case LAYER_ENVIRONMENT:
                return LAYER_PLAYER | LAYER_ENEMY | LAYER_PROJECTILE;
            case LAYER_PROJECTILE:
                return LAYER_ENEMY | LAYER_ENVIRONMENT;
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

        // ──────────────────────────────────────────────────────
        // NEW: Delete the btTriangleMesh objects.
        // These MUST be deleted AFTER the shapes that reference
        // them (the shapes were deleted in the loop above).
        // ──────────────────────────────────────────────────────
        for (auto& [key, triMesh] : ownedTriangleMeshes) {
            delete triMesh;
        }
        ownedTriangleMeshes.clear();

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
        // apply pushback for non-trigger collisions
        for (const CollisionEvent& event : frameCollisions) {
            // discard the trigger collisions since they don't need pushback
            auto* colliderA = event.entityA->getComponent<ColliderComponent>();
            auto* colliderB = event.entityB->getComponent<ColliderComponent>();

            if (!colliderA || !colliderB) continue;
            if (colliderA->isTrigger || colliderB->isTrigger) continue;

            // clang-format off
            // push back logic (don't push environments)
            if (colliderA->layer == CollisionLayer::LAYER_ENVIRONMENT) {
                event.entityB->localTransform.position -= worldToLocal(event.entityB, event.normal * event.penetrationDepth);
            } else if (colliderB->layer == CollisionLayer::LAYER_ENVIRONMENT) {
                event.entityA->localTransform.position += worldToLocal(event.entityA, event.normal * event.penetrationDepth);
            } else {  // this may be edited or removed later
                event.entityA->localTransform.position += worldToLocal(event.entityA, event.normal * (event.penetrationDepth / 2.0f));
                event.entityB->localTransform.position -= worldToLocal(event.entityB, event.normal * (event.penetrationDepth / 2.0f));
            }
            // clang-format on
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

    void CollisionSystem::addEntity(our::Entity* entity) {
        auto* collider = entity->getComponent<ColliderComponent>();
        if (!collider) return;

        // Create or reuse collision shape based on collider data
        btCollisionShape* shape = nullptr;

        if (collider->shape == ColliderShape::Mesh) {
            // ──────────────────────────────────────────────────
            // STEP 1: Determine the cache key.
            // For mesh shapes, we key on the model asset name
            // (e.g. "arena") instead of radius/height/scale.
            // This means if two entities share the same model,
            // they share the same BVH — no redundant rebuilds.
            // ──────────────────────────────────────────────────
            std::string meshKey = "mesh_" + collider->meshModelName;
            if (shapesCache.find(meshKey) != shapesCache.end()) {
                // ── Cache HIT: reuse the existing BVH shape ──
                shape = shapesCache[meshKey];
                ownedShapes[shape]++;
            } else {
                // ── Cache MISS: build the BVH from scratch ──
                // ──────────────────────────────────────────────
                // STEP 2: Get the Model from the AssetLoader.
                // The model name in the collider component must
                // match a key in your JSON's "models" section.
                //
                // Fallback: if meshModelName is empty, try to
                // get the model from a sibling ModelRendererComponent
                // on the same entity.
                // ──────────────────────────────────────────────
                our::Model* model = nullptr;
                if (!collider->meshModelName.empty()) {
                    model = our::AssetLoader<our::Model>::get(collider->meshModelName);
                }
                // Fallback: try the entity's own ModelRendererComponent
                if (!model) {
                    auto* renderer = entity->getComponent<our::ModelRendererComponent>();
                    if (renderer && renderer->model) {
                        model = renderer->model;
                    }
                }
                if (!model || !model->getCombinedMesh()) {
                    std::cerr << "[CollisionSystem] ERROR: Mesh collider on entity '"
                              << (entity->name.empty() ? "unnamed" : entity->name)
                              << "' but no model/combinedMesh found. Skipping." << std::endl;
                    return;
                }
                // ──────────────────────────────────────────────
                // STEP 3: Extract CPU-side vertex and index data.
                //
                // Your Model::generateCombinedMesh() already:
                //   • merges all submeshes
                //   • transforms each vertex by its submesh transform
                //   • re-bases indices into the combined buffer
                //
                // So the data we get here is in MODEL SPACE.
                // Since we place the entity at the origin with
                // identity rotation, model space == world space.
                // ──────────────────────────────────────────────
                our::Mesh* combinedMesh = model->getCombinedMesh();
                const std::vector<our::Vertex>& vertices = combinedMesh->getVertices();
                const std::vector<unsigned int>& indices = combinedMesh->getIndices();
                // ──────────────────────────────────────────────
                // STEP 4: Create a btTriangleMesh.
                //
                // btTriangleMesh is Bullet's container for
                // arbitrary triangle soup. We feed it triangles
                // one by one (3 vertices per call).
                //
                // Constructor args:
                //   use32bitIndices = true  (matches our unsigned int)
                //   use4componentVertices = false (we use 3-component)
                // ──────────────────────────────────────────────
                btTriangleMesh* triangleMesh = new btTriangleMesh(
                    /* use32bitIndices = */ true,
                    /* use4componentVertices = */ false);
                // ──────────────────────────────────────────────
                // STEP 5: Feed triangles into btTriangleMesh.
                //
                // The indices array is a flat list where every
                // 3 consecutive values form one triangle:
                //   [i0, i1, i2,  i3, i4, i5,  ...]
                //    ╰─ tri 0 ─╯  ╰─ tri 1 ─╯
                //
                // For each triangle, we:
                //   1. Look up the 3 vertex positions by index
                //   2. Convert each glm::vec3 → btVector3
                //   3. Call addTriangle(v0, v1, v2)
                // ──────────────────────────────────────────────
                size_t triCount = indices.size() / 3;
                for (size_t i = 0; i < triCount; ++i) {
                    // Extract the three vertex positions for this triangle
                    const glm::vec3& p0 = vertices[indices[i * 3 + 0]].position;
                    const glm::vec3& p1 = vertices[indices[i * 3 + 1]].position;
                    const glm::vec3& p2 = vertices[indices[i * 3 + 2]].position;
                    // Convert from glm to Bullet's vector type
                    btVector3 v0(p0.x, p0.y, p0.z);
                    btVector3 v1(p1.x, p1.y, p1.z);
                    btVector3 v2(p2.x, p2.y, p2.z);
                    // Feed the triangle to Bullet.
                    // removeDuplicateVertices = false → faster build,
                    // our mesh is already optimized by Assimp's
                    // aiProcess_JoinIdenticalVertices.
                    triangleMesh->addTriangle(v0, v1, v2, /* removeDuplicateVertices = */ false);
                }
                std::cout << "[CollisionSystem] Built triangle mesh for '" << collider->meshModelName
                          << "': " << triCount << " triangles, " << vertices.size() << " vertices." << std::endl;
                // ──────────────────────────────────────────────
                // STEP 6: Create the btBvhTriangleMeshShape.
                //
                // This is where the BVH tree is actually built.
                // The second argument (useQuantizedAabbCompression)
                // = true means Bullet will use 16-bit quantized
                // AABBs internally → ~4x less memory for the BVH
                // nodes, with negligible precision loss.
                //
                // This call is expensive (it builds the entire
                // tree), but it only happens ONCE thanks to our
                // caching strategy.
                // ──────────────────────────────────────────────
                shape = new btBvhTriangleMeshShape(triangleMesh,
                                                   /* useQuantizedAabbCompression = */ true);
                // ──────────────────────────────────────────────
                // STEP 7: Cache both the shape AND the triangle
                // mesh so we can:
                //   a) Reuse the shape if the scene reloads
                //   b) Properly delete the btTriangleMesh in
                //      destroy() — it must outlive the shape!
                // ──────────────────────────────────────────────
                shapesCache[meshKey] = shape;
                ownedShapes[shape] = 1;
                ownedTriangleMeshes[meshKey] = triangleMesh;  // prevent premature deletion
            }
        } else {
            // Shape_r_h_Scale_x_y_z
            std::string shapeKey =
                std::to_string(static_cast<int>(collider->shape)) + "_" + std::to_string(collider->radius) + "_" +
                std::to_string(collider->height) + "_Scale_" + std::to_string(entity->localTransform.scale.x) + "_" +
                std::to_string(entity->localTransform.scale.y) + "_" + std::to_string(entity->localTransform.scale.z);
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
                }
                shape->setLocalScaling(glmToBtVec3(entity->localTransform.scale));
                shapesCache[shapeKey] = shape;
                ownedShapes[shape] = 1;
            }
        }
        // Create the collision object
        btCollisionObject* obj = new btCollisionObject();
        obj->setCollisionShape(shape);
        obj->setWorldTransform(entityToBtTransform(entity));
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

        obj->setWorldTransform(entityToBtTransform(entity));

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

            switch (collider->shape) {
                case ColliderShape::Sphere:
                    debugDrawer->drawSphereWireframe(transform, scaledRadius, color);
                    break;
                case ColliderShape::Capsule:
                    debugDrawer->drawCapsuleWireframe(transform, scaledRadius, scaledHeight, color);
                    break;
                case ColliderShape::Mesh: {
                    btVector3 btColor(color.x, color.y, color.z);
                    collisionWorld->debugDrawObject(bt, obj->getCollisionShape(), btColor);
                    break;
                }
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
