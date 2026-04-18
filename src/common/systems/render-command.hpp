#pragma once
#include "material/material.hpp"
#include "mesh/mesh.hpp"

namespace our {
    // The render command stores command that tells the renderer that it should draw
    // the given mesh at the given localToWorld matrix using the given material
    // The renderer will fill this struct using the mesh renderer components
    struct RenderCommand {
        glm::mat4 localToWorld;
        glm::vec3 center;
        Mesh* mesh;
        Material* material;
    };
};  // namespace our
