#include "collider.hpp"

#include <btBulletCollisionCommon.h>

#include "asset-loader.hpp"
#include "deserialize-utils.hpp"

static gameplay::ColliderShape parseColliderShape(const std::string& value, gameplay::ColliderShape fallback) {
    if (value == "Sphere") return gameplay::ColliderShape::Sphere;
    if (value == "Capsule") return gameplay::ColliderShape::Capsule;
    if (value == "Mesh" || value == "Model") return gameplay::ColliderShape::Mesh;
    return fallback;
}

static inline gameplay::CollisionLayer layerStringToGroup(const std::string& layer) {
    if (layer == "player") return gameplay::CollisionLayer::LAYER_PLAYER;
    if (layer == "enemy") return gameplay::CollisionLayer::LAYER_ENEMY;
    if (layer == "projectile") return gameplay::CollisionLayer::LAYER_PROJECTILE;
    if (layer == "trigger") return gameplay::CollisionLayer::LAYER_TRIGGER;
    return gameplay::CollisionLayer::LAYER_ENVIRONMENT;  // environment is the default layer
}

namespace gameplay {

    void ColliderComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        shape = parseColliderShape(data.value("shape", std::string("Sphere")), shape);
        radius = data.value("radius", radius);
        height = data.value("height", height);
        isTrigger = data.value("isTrigger", isTrigger);
        worldSpace = data.value("worldSpace", worldSpace);
        if (data.contains("centerOffset") && data["centerOffset"].is_array()) {
            centerOffset = data["centerOffset"].get<glm::vec3>();
        }
        std::string stringLayer = data.value("layer", "");
        layer = layerStringToGroup(stringLayer);
        std::string modelName = data.value("model", "");
        std::string meshName =
            data.value("mesh", "");  // "mesh" is an alternative key for the model name, in case "model" is not provided
        if (!modelName.empty()) {
            our::Model* model = our::AssetLoader<our::Model>::get(modelName);
            if (model) {
                mesh = model->getCombinedMesh();
            }
        } else if (!meshName.empty()) {
            our::Mesh* assetMesh = our::AssetLoader<our::Mesh>::get(meshName);
            if (assetMesh) {
                mesh = assetMesh;
            }
        }
    }

}  // namespace gameplay
