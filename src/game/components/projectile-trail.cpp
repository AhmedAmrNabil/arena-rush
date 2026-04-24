#include "projectile-trail.hpp"

namespace gameplay {

    void ProjectileTrailComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        material = data.value("material", material);
        mesh = data.value("mesh", mesh);
        maxSegments = data.value("maxSegments", maxSegments);
        segmentLifetime = data.value("segmentLifetime", segmentLifetime);
        headScale = data.value("headScale", headScale);
        tailScale = data.value("tailScale", tailScale);
    }

}  // namespace gameplay
