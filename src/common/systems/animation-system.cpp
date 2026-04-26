#include "animation-system.hpp"

#include <string>

#include "components/animation.hpp"

namespace our {
    void AnimationSystem::update(World* world, float deltaTime) {
        for (auto entity : world->getEntities()) {
            AnimationComponent* animComp = entity->getComponent<AnimationComponent>();
            if (!animComp) continue;

            AnimationCommand from = animComp->getCurrentCommand();
            AnimationCommand to = animComp->getQueuedCommand();
            if (animComp->canTransitionNow()) {
                if (from.to == AnimationState::Attack && to.to == AnimationState::Walk) {
                    if (animComp->play("attack_then_walk")) {
                        animComp->setNextCommand(to);
                    } else {
                        animComp->setCommand(to);
                    }
                } else if (from.to == AnimationState::Attack &&
                           (to.to == AnimationState::Idle || to.to == AnimationState::Attack)) {
                    if (animComp->play("attack_then_idle")) {
                        animComp->setNextCommand(to);
                    } else {
                        animComp->setCommand(to);
                    }
                } else {
                    animComp->setCommand(to);
                }
            } else {
                // this skips the attack then idle animation and goes straight to attack then walk instead
                if (animComp->getCurrentCommand().to == AnimationState::AttackThenIdle &&
                    to.to == AnimationState::Walk) {
                    if (animComp->play("attack_then_walk", 1.0f, false)) {
                        animComp->setNextCommand(to);
                    } else {
                        animComp->setCommand(to);
                    }
                }
            }

            animComp->animator.update(deltaTime);
        }
    }
}  // namespace our
