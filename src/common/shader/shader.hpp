#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace our {

    class ShaderProgram {
    private:
        // Shader Program Handle (OpenGL object name)
        GLuint program;

    public:
        ShaderProgram() {
            this->program = glCreateProgram();
        }
        ~ShaderProgram() {
            glDeleteProgram(this->program);
        }

        bool attach(const std::string& filename, GLenum type) const;

        bool link() const;

        void use() {
            glUseProgram(program);
        }

        GLuint getUniformLocation(const std::string& name) {
            return glGetUniformLocation(this->program, name.c_str());
        }

        void set(const std::string& uniform, GLfloat value) {
            glUniform1f(this->getUniformLocation(uniform), value);
        }

        void set(const std::string& uniform, GLuint value) {
            glUniform1ui(this->getUniformLocation(uniform), value);
        }

        void set(const std::string& uniform, GLint value) {
            glUniform1i(this->getUniformLocation(uniform), value);
        }

        void set(const std::string& uniform, glm::vec2 value) {
            glUniform2fv(this->getUniformLocation(uniform), 1, glm::value_ptr(value));
        }

        void set(const std::string& uniform, glm::vec3 value) {
            glUniform3fv(this->getUniformLocation(uniform), 1, glm::value_ptr(value));
        }

        void set(const std::string& uniform, glm::vec4 value) {
            glUniform4fv(this->getUniformLocation(uniform), 1, glm::value_ptr(value));
        }

        void set(const std::string& uniform, glm::mat4 matrix) {
            glUniformMatrix4fv(this->getUniformLocation(uniform), 1, GL_FALSE, glm::value_ptr(matrix));
        }

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        // Question: Why do we delete the copy constructor and assignment operator?
        // To avoid dangling pointers and double deletion issues that can arise from (shallow) copying shader programs,
        // which may result in deletion of the program in 1 place and still referencing in another, and prevent general
        // resource management bugs.
    };

}  // namespace our

#endif
