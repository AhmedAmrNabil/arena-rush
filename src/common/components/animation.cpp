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

        // deserializing clip:
        if (data.contains("clips")) {
            for (auto& [name, clipData] : data["clips"].items()) {
                AnimationClip clip;

                if (!clipData.contains("clip")) {
                    std::cerr << "\033[31mClip missing 'clip' field: " << name << "\033[0m" << std::endl;
                    continue;
                }

                clip.clip = clipData["clip"].get<std::string>();
                if (clipData.contains("loop")) clip.loop = clipData["loop"].get<bool>();
                if (clipData.contains("speed")) clip.speed = clipData["speed"].get<float>();
                clips[name] = clip;
                std::cout << "Loaded animation clip: " << name << " -> " << clip.clip << " (loop: " << clip.loop
                          << ", speed: " << clip.speed << ")" << std::endl;
            }
        }
        // default clip is the clip that will be played when the animation component is first initialized
        defaultClip = data.value("default", "");
        if (!defaultClip.empty()) {
            play(defaultClip);
        }
    }

    bool AnimationComponent::play(const std::string& clipName, float speedScale) {
        if (!model) {
            std::cerr << "\033[31mCannot play animation: model is not set\033[0m" << std::endl;
            return false;
        }

        auto clipIt = clips.find(clipName);
        if (clipIt == clips.end()) {
            std::cerr << "\033[31mClip not found: " << clipName << "\033[0m" << std::endl;
            return false;
        }
        AnimationClip& clip = clipIt->second;
        auto animIt = model->animations.find(clip.clip);
        if (animIt == model->animations.end()) {
            std::cerr << "\033[31mAnimation not found in model: " << clip.clip << "\033[0m" << std::endl;
            return false;
        }
        // speed scale is used so we can change the speed of the animation
        // depending on the movement speed of the entity
        // and the scale of the model (eg. a giant monster should have slower animations than a small character)
        currentClip = clipName;
        animator.play(&animIt->second, clip.loop, clip.speed * speedScale);
        return true;
    }

    bool AnimationComponent::playDuration(const std::string& clipName, float duration) {
        if (!model) {
            std::cerr << "\033[31mCannot play animation: model is not set\033[0m" << std::endl;
            return false;
        }

        auto clipIt = clips.find(clipName);
        if (clipIt == clips.end()) {
            std::cerr << "\033[31mClip not found: " << clipName << "\033[0m" << std::endl;
            return false;
        }
        AnimationClip& clip = clipIt->second;
        auto animIt = model->animations.find(clip.clip);
        if (animIt == model->animations.end()) {
            std::cerr << "\033[31mAnimation not found in model: " << clip.clip << "\033[0m" << std::endl;
            return false;
        }
        float originalSpeed = clip.speed;
        float animDuration = animIt->second.duration / animIt->second.ticksPerSecond;
        float speedScale = animDuration > 0.0f ? animDuration / duration : 1.0f;

        currentClip = clipName;
        animator.play(&animIt->second, false, originalSpeed * speedScale);
        return true;
    }

}  // namespace our
