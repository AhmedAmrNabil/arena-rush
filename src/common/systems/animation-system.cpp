#include "animation-system.hpp"

#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include "components/animation.hpp"

namespace our {
    void AnimationSystem::update(World* world, float deltaTime) {
        for (auto entity : world->getEntities()) {
            AnimationComponent* animComp = entity->getComponent<AnimationComponent>();
            if (!animComp) continue;
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE) == GLFW_PRESS) {
                animComp->play("Snake_Attack");
            }
            animComp->animator.update(deltaTime * animComp->speed, animComp->loop);
        }
    }
}  // namespace our
