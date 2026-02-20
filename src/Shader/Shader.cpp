#include <fstream>
#include <cstring>

#include "Shader.h"

Shader::Shader() : ID(0) {
    ID = glCreateProgram();
}

Shader& Shader::addShader(const char* path, GLenum type) {
    std::ifstream shaderFile(path);
    if (!shaderFile.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + std::string(path));
    }

    std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    const char* sourceCStr = source.c_str();

    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    checkCompileErrors(shader, type);

    glAttachShader(ID, shader);
    glDeleteShader(shader);  // Flag for deletion after linking
    return *this;
}

void Shader::link() {
    glLinkProgram(ID);
    checkCompileErrors(ID, GL_PROGRAM);
}

void Shader::use() {
    glUseProgram(ID);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

std::string shaderTypeToString(GLenum type) {
    switch (type) {
        case GL_VERTEX_SHADER:
            return "VERTEX";
        case GL_FRAGMENT_SHADER:
            return "FRAGMENT";
        case GL_PROGRAM:
            return "PROGRAM";
        default:
            return "UNKNOWN";
    }
}

void Shader::checkCompileErrors(unsigned int shader, GLenum type) {
    int success;
    char infoLog[1024];
    if (type == GL_PROGRAM) {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            throw std::runtime_error("Shader program linking error: " + std::string(infoLog));
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            throw std::runtime_error(shaderTypeToString(type) + " shader compilation error: " + std::string(infoLog));
        }
    }
}