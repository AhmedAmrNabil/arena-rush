#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>

namespace our {

    enum class TextureUnits {
        ALBEDO = 0,
        METALLIC = 1,
        ROUGHNESS = 2,
        NORMAL = 3,
        AMBIENT_OCCLUSION = 4,
        EMISSIVE = 5,
        METALLIC_ROUGHNESS = 6
    };

    // This class defined an OpenGL texture which will be used as a GL_TEXTURE_2D
    class Texture2D {
        // The OpenGL object name of this texture
        GLuint name = 0;

        // Needed for the text renderer, we use atlas textures so we calculate the UVs from the size
        int width = 0;
        int height = 0;

    public:
        // This constructor creates an OpenGL texture
        Texture2D() {
            glGenTextures(1, &name);
        };

        // This deconstructor deletes the underlying OpenGL texture
        ~Texture2D() {
            glDeleteTextures(1, &name);
        }

        void setSize(int w, int h) {
            width = w;
            height = h;
        }
        int getWidth() const {
            return width;
        }
        int getHeight() const {
            return height;
        }
        glm::ivec2 getSize() const {
            return {width, height};
        }

        // Get the internal OpenGL name of the texture which is useful for use with framebuffers
        GLuint getOpenGLName() const {
            return name;
        }

        // This method binds this texture to GL_TEXTURE_2D
        void bind() const {
            glBindTexture(GL_TEXTURE_2D, name);
        }

        // This static method ensures that no texture is bound to GL_TEXTURE_2D
        static void unbind() {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;
    };

}  // namespace our
