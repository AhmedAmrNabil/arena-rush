#include "collider.hpp"

static gameplay::ColliderShape parseColliderShape(const std::string& value, gameplay::ColliderShape fallback) {
    if (value == "Sphere") return gameplay::ColliderShape::Sphere;
    if (value == "Capsule") return gameplay::ColliderShape::Capsule;
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
        std::string stringLayer = data.value("layer", "");
        layer = layerStringToGroup(stringLayer);
    }

}  // namespace gameplay
