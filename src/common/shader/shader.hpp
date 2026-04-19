#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

namespace our {

    class ShaderProgram {
    private:
        // Shader Program Handle
        GLuint program;
        mutable std::unordered_map<std::string, GLint> uniformLocations;

    public:
        ShaderProgram() {
            program = glCreateProgram();
        }
        ~ShaderProgram() {
            glDeleteProgram(program);
        }

        bool attach(const std::string& filename, GLenum type) const;

        bool link() const;

        void use() {
            glUseProgram(program);
        }

        GLint getUniformLocation(const std::string& name) {
            if (uniformLocations.find(name) != uniformLocations.end()) return uniformLocations[name];

            GLint location = glGetUniformLocation(program, name.c_str());
            uniformLocations[name] = location;
            return location;
        }

        void set(const std::string& uniform, GLfloat value) {
            glUniform1f(getUniformLocation(uniform), value);
        }

        void set(const std::string& uniform, GLuint value) {
            glUniform1ui(getUniformLocation(uniform), value);
        }

        void set(const std::string& uniform, GLint value) {
            glUniform1i(getUniformLocation(uniform), value);
        }

        void set(const std::string& uniform, glm::vec2 value) {
            glUniform2fv(getUniformLocation(uniform), 1, glm::value_ptr(value));
        }

        void set(const std::string& uniform, glm::vec3 value) {
            glUniform3fv(getUniformLocation(uniform), 1, glm::value_ptr(value));
        }

        void set(const std::string& uniform, glm::vec4 value) {
            glUniform4fv(getUniformLocation(uniform), 1, glm::value_ptr(value));
        }

        void set(const std::string& uniform, glm::mat4 matrix) {
            glUniformMatrix4fv(getUniformLocation(uniform), 1, GL_FALSE, glm::value_ptr(matrix));
        }

        void bindUniformBlock(const std::string& blockName, GLuint binding) {
            GLuint index = glGetUniformBlockIndex(program, blockName.c_str());
            if (index != GL_INVALID_INDEX) {
                glUniformBlockBinding(program, index, binding);
            }
        }

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        // Question: Why do we delete the copy constructor and assignment operator?
        //  as we cannot copy the program generated form the opengl
        //  so we don't want accidental copying of the ShaderProgram as this can cause
        //  undefined behavior when the destructor is called on the opengl program
    };

}  // namespace our

#endif
