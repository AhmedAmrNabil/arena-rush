#include "animation-system.hpp"

#include <string>

#include "components/animation.hpp"

namespace our {

    struct TransitionRule {
        AnimationState from;
        AnimationState to;
        const char* clip;
        bool loop;
    };

    const TransitionRule transitionRules[] = {
        {AnimationState::Attack, AnimationState::Walk, "attack_then_walk", true},
        {AnimationState::Attack, AnimationState::Idle, "attack_then_idle", true},
        {AnimationState::Attack, AnimationState::Attack, "attack_then_idle", true},
    };

    const TransitionRule overrideRules[] = {
        {AnimationState::AttackThenIdle, AnimationState::Walk, "attack_then_walk", false},
    };

    void AnimationSystem::update(World* world, float deltaTime) {
        for (auto entity : world->getEntities()) {
            AnimationComponent* animComp = entity->getComponent<AnimationComponent>();
            if (!animComp) continue;

            AnimationCommand from = animComp->getCurrentCommand();
            AnimationCommand to = animComp->getQueuedCommand();
            const AnimationState fromState = from.to;
            const AnimationState toState = to.to;

            auto applyRule = [&](const TransitionRule& rule) -> bool {
                if (fromState != rule.from || toState != rule.to) return false;
                if (animComp->play(rule.clip, 1.0f, rule.loop)) {
                    animComp->setNextCommand(to);
                } else {
                    animComp->setCommand(to);
                }
                return true;
            };

            if (animComp->canTransitionNow()) {
                bool handled = false;
                for (const auto& rule : transitionRules) {
                    if (applyRule(rule)) {
                        handled = true;
                        break;
                    }
                }
                if (!handled) animComp->setCommand(to);
            } else {
                for (const auto& rule : overrideRules) {
                    if (applyRule(rule)) break;
                }
            }

            animComp->animator.update(deltaTime);
        }
    }
}  // namespace our
