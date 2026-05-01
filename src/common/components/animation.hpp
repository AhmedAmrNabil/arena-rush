#pragma once

#include <string>

#include "animation/animator.hpp"
#include "ecs/component.hpp"
#include "model/model.hpp"

namespace our {
    enum class AnimationState { Idle, Walk, Attack, Death, AttackThenWalk, AttackThenIdle };

    struct AnimationCommand {
        AnimationState to;
        float duration = -1.0f;   // in seconds, if -1 then play with speed specified in the clip, otherwise adjust the
                                  // speed to fit the duration
        float speedScale = 1.0f;  // optional speed scale for this command (default is 1, meaning no change to the
                                  // original animation speed)
    };

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
        std::string currentClip = "";
        AnimationCommand currentCommand = {AnimationState::Idle, -1.0f, 1.0f};
        AnimationCommand nextCommand = {AnimationState::Idle, -1.0f, 1.0f};
        bool hasNextState = false;
        // maps clip names to their corresponding animation clip data (animation name, loop, speed)
        std::unordered_map<std::string, AnimationClip> clips;
        static std::string getID() {
            return "Animation";
        }

        void deserialize(const nlohmann::json& data) override;

        bool play(const std::string& clipName, float speedScale = 1.0f, bool resetCurrentTime = true);
        bool playDuration(const std::string& clipName, float duration);
        bool playAttack(float duration);

        static const char* stateToClip(AnimationState state);
        void setCommand(AnimationCommand command);
        void setState(AnimationState state) {
            setCommand({state, -1.0f, 1.0f});
        }
        void setNextCommand(AnimationCommand command);
        void setNextState(AnimationState state) {
            setNextCommand({state, -1.0f, 1.0f});
        }

        AnimationCommand getCurrentCommand() const {
            return currentCommand;
        }

        AnimationCommand getQueuedCommand() const {
            return nextCommand;
        }

        AnimationState getState() const {
            return currentCommand.to;
        }
        bool hasQueuedState() const {
            return hasNextState;
        }
        AnimationState getQueuedState() const {
            return nextCommand.to;
        }
        void clearQueuedState() {
            hasNextState = false;
        }
        bool canTransitionNow() const;
    };

}  // namespace our
