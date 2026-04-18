#pragma once
#include <assimp/scene.h>

#include <glm/glm.hpp>

namespace our {
    inline glm::mat4 aiToGlm(const aiMatrix4x4& mat) {
        // clang-format off
        return glm::mat4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        );
        // clang-format on
    }

    inline glm::vec3 aiToGlm(const aiVector3D& vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    inline glm::vec2 aiToGlm(const aiVector2D& vec) {
        return glm::vec2(vec.x, vec.y);
    }

    inline glm::vec4 aiToGlm(const aiColor4D& color) {
        return glm::vec4(color.r, color.g, color.b, color.a);
    }
}  // namespace our
