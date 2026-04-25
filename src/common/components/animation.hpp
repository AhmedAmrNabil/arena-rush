#pragma once

#include <string>

#include "animation/animator.hpp"
#include "ecs/component.hpp"
#include "model/model.hpp"

namespace our {
    struct AnimationClip {
        std::string clip;
        bool loop = false;
        float speed = 1.0f;
    };

    class AnimationComponent : public Component {
    public:
        Model* model = nullptr;
        Animator animator;
        std::string defaultClip = "";
        // maps clip names to their corresponding animation clip data (animation name, loop, speed)
        std::unordered_map<std::string, AnimationClip> clips;
        static std::string getID() {
            return "Animation";
        }

        void deserialize(const nlohmann::json& data) override;

        void play(const std::string& clipName, float speedScale = 1.0f);
        void playDuration(const std::string& clipName, float duration);
    };

}  // namespace our
