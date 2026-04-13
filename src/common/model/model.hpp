#pragma once

#include <iostream>
#include <json/json.hpp>
#include <mesh/mesh.hpp>
#include <shader/shader.hpp>
#include <texture/sampler.hpp>
#include <texture/texture2d.hpp>

namespace our {

    struct TextureBinding {
        Texture2D* texture;
        std::string uniform;
        int unit = 0;
    };

    struct SubMeshMaterial {
        glm::vec4 diffuse = {1, 1, 1, 1};
        glm::vec4 ambient = {0.1f, 0.1f, 0.1f, 1};
        glm::vec4 specular = {0.5f, 0.5f, 0.5f, 1};
        glm::vec4 emissive = {0, 0, 0, 1};
        float shininess = 32.0f;
    };

    struct SubMesh {
        Mesh* mesh;
        std::vector<TextureBinding> textureBindings;
        SubMeshMaterial material;
        glm::mat4 transform;
        SubMesh(Mesh* mesh, std::vector<TextureBinding> textureBindings, SubMeshMaterial material, glm::mat4 transform)
            : mesh(mesh), textureBindings(textureBindings), material(material), transform(transform) {}
    };

    class Model {
    public:
        Model() = default;
        std::vector<SubMesh> submeshes;
        void draw(ShaderProgram* shader, const glm::mat4& MVP) const {
            for (const auto& submesh : submeshes) {
                shader->set("material.diffuse", submesh.material.diffuse);
                shader->set("material.ambient", submesh.material.ambient);
                shader->set("material.specular", submesh.material.specular);
                shader->set("material.emissive", submesh.material.emissive);
                shader->set("material.shininess", submesh.material.shininess);
                shader->set("transform", MVP * submesh.transform);
                for (const auto& textureBinding : submesh.textureBindings) {
                    glActiveTexture(GL_TEXTURE0 + textureBinding.unit);
                    textureBinding.texture->bind();
                    shader->set(textureBinding.uniform, textureBinding.unit);
                }
                submesh.mesh->draw();
            }
            glActiveTexture(GL_TEXTURE0);
        }
        ~Model() {
            for (auto& submesh : submeshes) {
                delete submesh.mesh;
            }
        }
    };

}  // namespace our
