#pragma once
#include "ecs/world.hpp"

namespace our {

    class AnimationSystem {
    public:
        void update(World* world, float deltaTime);
    };
}  // namespace our
