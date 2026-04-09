#pragma once

#include <glad/gl.h>

namespace our {

    // This class defined an OpenGL texture which will be used as a GL_TEXTURE_2D
    class Texture2D {
        // The OpenGL object name of this texture
        GLuint name = 0;

    public:
        // This constructor creates an OpenGL texture
        Texture2D() {
            glCreateTextures(GL_TEXTURE_2D, 1, &name);
        };

        // This deconstructor deletes the underlying OpenGL texture
        ~Texture2D() {
            glDeleteTextures(1, &name);
        }

        // Get the internal OpenGL name of the texture which is useful for use with framebuffers
        GLuint getOpenGLName() {
            return name;
        }

        // Those bind and unbind are the modern way to do it by explicitly passing a texture unit, instead of relying on
        // the global active GL_TEXTURE_2D. This is much less error prone.
        // Also this makes more sense, given how the bind() and unbind() functions of the Sampler work.
        void bind(GLuint unit = 0) const {
            glBindTextureUnit(unit, name);
        }

        static void unbind(GLuint unit = 0) {
            glBindTextureUnit(unit, 0);
        }

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;
    };

}  // namespace our
