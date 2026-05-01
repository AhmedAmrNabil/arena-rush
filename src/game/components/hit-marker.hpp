#pragma once

#include <ecs/component.hpp>
#include <material/material.hpp>

namespace gameplay {

    class HitMarkerComponent : public our::Component {
    public:
        float age = 0.0f;
        float lifetime = 0.3f;
        float startScale = 0.7f;
        float endScale = 0.1f;
        float startAlpha = 1.0f;
        float endAlpha = 0.0f;
        our::Material* ownedMaterial = nullptr;

        static std::string getID() {
            return "Hit Marker";
        }

        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;
            lifetime = data.value("lifetime", lifetime);
            startScale = data.value("startScale", startScale);
            endScale = data.value("endScale", endScale);
            startAlpha = data.value("startAlpha", startAlpha);
            endAlpha = data.value("endAlpha", endAlpha);
        }
    };

}  // namespace gameplay
