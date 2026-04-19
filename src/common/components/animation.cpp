#include "animation.hpp"

namespace our {

    void AnimationComponent::deserialize(const nlohmann::json& data) {
        if (!data.contains("model")) {
            std::cerr << "\033[31mAnimation component must specify a model\033[0m" << std::endl;
            return;
        }
        model = AssetLoader<Model>::get(data["model"].get<std::string>());
        if (!model) {
            std::cerr << "\033[31mModel not found for animation component: " << data["model"].get<std::string>()
                      << "\033[0m" << std::endl;
            return;
        }
        autoplay = data.value("autoplay", autoplay);
        loop = data.value("loop", loop);
        speed = data.value("speed", speed);
        clip = data.value("clip", "");
        std::cout << "lodead animationComponent: " << clip << std::endl;
        if (!clip.empty()) {
            auto it = model->animations.find(clip);
            if (it != model->animations.end()) {
                if (autoplay) animator.play(&it->second);
            } else {
                std::cerr << "\033[31mAnimation clip not found: " << clip << "\033[0m" << std::endl;
            }
        }
    }

    void AnimationComponent::play(const std::string& clipName) {
        if (!model) {
            std::cerr << "\033[31mCannot play animation: model is not set\033[0m" << std::endl;
            return;
        }
        auto it = model->animations.find(clipName);
        if (it != model->animations.end()) {
            animator.play(&it->second);
        } else {
            std::cerr << "\033[31mAnimation clip not found: " << clipName << "\033[0m" << std::endl;
        }
    }
}  // namespace our
