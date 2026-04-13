#include "collider.hpp"

static gameplay::ColliderShape parseColliderShape(const std::string& value, gameplay::ColliderShape fallback) {
    if (value == "Sphere") return gameplay::ColliderShape::Sphere;
    if (value == "Capsule") return gameplay::ColliderShape::Capsule;
    return fallback;
}

namespace gameplay {

    void ColliderComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        shape = parseColliderShape(data.value("shape", std::string("Sphere")), shape);
        layer = data.value("layer", layer);
        radius = data.value("radius", radius);
        height = data.value("height", height);
        isTrigger = data.value("isTrigger", isTrigger);
    }

}  // namespace gameplay
