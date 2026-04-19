#pragma once

// clang-format off
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
// clang-format on
#include "ecs/world.hpp"

namespace our {

    class AnimationSystem {
    public:
        void update(World* world, float deltaTime);
    };
}  // namespace our
