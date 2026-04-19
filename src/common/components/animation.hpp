#pragma once

#include <string>

#include "animation/animator.hpp"
#include "ecs/component.hpp"
#include "model/model.hpp"

namespace our {
    class AnimationComponent : public Component {
    public:
        Model* model;
        Animator animator;
        std::string clip;
        bool autoplay = false;
        bool loop = false;
        float speed = 1.0f;
        static std::string getID() {
            return "Animation";
        }

        void deserialize(const nlohmann::json& data) override;

        void play(const std::string& clipName);
    };

}  // namespace our
