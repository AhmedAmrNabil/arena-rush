#pragma once

#include <json/json.hpp>
#include <mesh/mesh.hpp>
#include <texture/sampler.hpp>
#include <texture/texture2d.hpp>

namespace our {

    struct TextureBinding {
        Texture2D* texture;
        std::string uniform;  // e.g. "u_albedo", "u_normal"
        int unit = 0;         // which GL_TEXTURE0+N to bind to
    };

    struct SubMesh {
        Mesh* mesh;
        std::vector<TextureBinding> textureBindings;
        SubMesh(Mesh* mesh, std::vector<TextureBinding> textureBindings)
            : mesh(mesh), textureBindings(textureBindings) {}
    };

    class Model {
    public:
        Model() = default;
        std::vector<SubMesh> submeshes;
        void draw() const {
            for (const auto& submesh : submeshes) {
                for (const auto& textureBinding : submesh.textureBindings) {
                    glActiveTexture(GL_TEXTURE0 + textureBinding.unit);
                    textureBinding.texture->bind();
                }
                submesh.mesh->draw();
            }
        }
        ~Model() {
            for (auto& submesh : submeshes) {
                delete submesh.mesh;
            }
        }
    };

}  // namespace our
