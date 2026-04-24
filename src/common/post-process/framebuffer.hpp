#pragma once
#include "texture/texture2d.hpp"

namespace our {

    class Framebuffer {
    public:
        GLuint frameBuffer;
        Texture2D* colorTarget = nullptr;
        Texture2D* depthTarget = nullptr;

        Framebuffer() {
            glGenFramebuffers(1, &frameBuffer);
        }

        ~Framebuffer() {
            glDeleteFramebuffers(1, &frameBuffer);
            if (colorTarget) delete colorTarget;
            if (depthTarget) delete depthTarget;
        }

        void resize(glm::ivec2 size, float hdr = false) {
            if (!colorTarget) colorTarget = new Texture2D();
            if (!depthTarget) depthTarget = new Texture2D();

            glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

            colorTarget->bind();
            glTexImage2D(GL_TEXTURE_2D, 0, hdr ? GL_RGBA16F : GL_RGBA8, size.x, size.y, 0, GL_RGBA,
                         hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTarget->getOpenGLName(),
                                   0);

            depthTarget->bind();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                         nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

            Texture2D::unbind();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void bind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        }

        static void unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        Texture2D* getColorTarget() const {
            return colorTarget;
        }
        Texture2D* getDepthTarget() const {
            return depthTarget;
        }

        // Non-copyable, movable
        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;
    };
}  // namespace our
