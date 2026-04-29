#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class TrailSegmentComponent : public our::Component {
    public:
        float age = 0.0f;
        float lifetime = 0.12f;
        float startScale = 0.12f;
        float endScale = 0.0f;

        static std::string getID() {
            return "Trail Segment";
        }

        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;
            lifetime = data.value("lifetime", lifetime);
            startScale = data.value("startScale", startScale);
            endScale = data.value("endScale", endScale);
        }
    };

}  // namespace gameplay
