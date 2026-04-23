#pragma once

#include <glad/gl.h>

namespace our {

    class UniformBuffer {
        GLuint name;
        GLuint bindingPoint;
        GLsizeiptr bufferSize;

    public:
        UniformBuffer(GLsizeiptr size, GLuint bindingPoint) {
            glGenBuffers(1, &name);
            glBindBuffer(GL_UNIFORM_BUFFER, name);
            glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            this->bindingPoint = bindingPoint;
            this->bufferSize = size;
        }
        ~UniformBuffer() {
            glDeleteBuffers(1, &name);
        }
        void bind() const {
            glBindBuffer(GL_UNIFORM_BUFFER, name);
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, name);
        }
        void unbind() const {
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
        // make sure to bind the uniform buffer before calling this function
        void update(const void* data, GLsizeiptr size, GLintptr offset = 0) const {
            glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        }

        // disable copying
        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;
    };

}  // namespace our
