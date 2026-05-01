#include "animation.hpp"

namespace our {

    const char* AnimationComponent::stateToClip(AnimationState state) {
        switch (state) {
            case AnimationState::Idle:
                return "idle";
            case AnimationState::Walk:
                return "walk";
            case AnimationState::Attack:
                return "attack";
            case AnimationState::Death:
                return "death";
            case AnimationState::AttackThenWalk:
                return "attack_then_walk";
            case AnimationState::AttackThenIdle:
                return "attack_then_idle";
        }
        return "idle";
    }

    AnimationState clipToState(const std::string& clip) {
        if (clip == "idle") return AnimationState::Idle;
        if (clip == "walk") return AnimationState::Walk;
        if (clip == "attack") return AnimationState::Attack;
        if (clip == "attack_then_walk") return AnimationState::AttackThenWalk;
        if (clip == "attack_then_idle") return AnimationState::AttackThenIdle;
        if (clip == "death") return AnimationState::Death;
        return AnimationState::Idle;
    }

    void AnimationComponent::setCommand(AnimationCommand command) {
        currentCommand = command;
        hasNextState = false;
        if (command.duration > 0.0f) {
            playDuration(stateToClip(command.to), command.duration);
        } else {
            play(stateToClip(command.to), command.speedScale);
        }
    }

    void AnimationComponent::setNextCommand(AnimationCommand command) {
        if (command.to == currentCommand.to && currentCommand.to != AnimationState::Attack) {
            hasNextState = false;
            return;
        }
        nextCommand = command;
        hasNextState = true;
    }

    bool AnimationComponent::canTransitionNow() const {
        if (!hasNextState) return false;
        if (currentCommand.to == AnimationState::Death) return false;
        if (currentCommand.to == AnimationState::Attack && !animator.isFinished()) return false;
        if (currentCommand.to == nextCommand.to && currentCommand.to != AnimationState::Attack) return false;
        return animator.isLooping() || animator.isFinished() || currentClip.empty();
    }

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
            currentCommand = {clipToState(defaultClip), -1, 1};
        }
    }

    bool AnimationComponent::play(const std::string& clipName, float speedScale, bool resetCurrentTime) {
        if (!model) {
            std::cerr << "\033[31mCannot play animation: model is not set\033[0m" << std::endl;
            return false;
        }

        auto clipIt = clips.find(clipName);
        if (clipIt == clips.end()) {
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
        currentCommand = {clipToState(clipName), -1, speedScale};
        hasNextState = false;
        animator.play(&animIt->second, clip.loop, clip.speed * speedScale, resetCurrentTime);
        return true;
    }

    bool AnimationComponent::playDuration(const std::string& clipName, float duration) {
        if (!model) {
            std::cerr << "\033[31mCannot play animation: model is not set\033[0m" << std::endl;
            return false;
        }

        auto clipIt = clips.find(clipName);
        if (clipIt == clips.end()) {
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
        currentCommand = {clipToState(clipName), duration, speedScale};
        hasNextState = false;
        animator.play(&animIt->second, false, originalSpeed * speedScale);
        return true;
    }

    bool AnimationComponent::playAttack(float duration) {
        if (!model) {
            std::cerr << "\033[31mCannot play animation: model is not set\033[0m" << std::endl;
            return false;
        }

        auto attackAnimationClipName = clips.find("attack");
        if (attackAnimationClipName == clips.end()) {
            return false;
        }

        AnimationClip& clip = attackAnimationClipName->second;
        auto attackAnimationIter = model->animations.find(clip.clip);
        if (attackAnimationIter == model->animations.end()) {
            std::cerr << "\033[31mAnimation not found in model: " << clip.clip << "\033[0m" << std::endl;
            return false;
        }

        Animation& attackAnimation = attackAnimationIter->second;

        float totalDuration = attackAnimation.duration / attackAnimation.ticksPerSecond;

        auto attackThenIdleAnimationClipName = clips.find("attack_then_idle");
        if (attackThenIdleAnimationClipName != clips.end()) {
            auto attackThenIdleAnimationIter = model->animations.find(attackThenIdleAnimationClipName->second.clip);
            if (attackThenIdleAnimationIter != model->animations.end()) {
                totalDuration +=
                    attackThenIdleAnimationIter->second.duration / attackThenIdleAnimationIter->second.ticksPerSecond;
            }
        }
        float speedScale = totalDuration > 0.0f ? totalDuration / duration : 1.0f;
        currentClip = "attack";
        currentCommand = {AnimationState::Attack, -1, speedScale};
        hasNextState = false;
        animator.play(&attackAnimation, false, clip.speed * speedScale);
        return true;
    }

}  // namespace our
