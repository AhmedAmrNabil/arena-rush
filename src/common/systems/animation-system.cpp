#include "animation-system.hpp"

#include <string>

#include "components/animation.hpp"

namespace our {
    void AnimationSystem::update(World* world, float deltaTime) {
        for (auto entity : world->getEntities()) {
            AnimationComponent* animComp = entity->getComponent<AnimationComponent>();
            if (!animComp) continue;
            animComp->animator.update(deltaTime);

            if (animComp->animator.isFinished()) {
                if (!animComp->defaultClip.empty()) {
                    animComp->play(animComp->defaultClip);
                }
            }
        }
    }
}  // namespace our
