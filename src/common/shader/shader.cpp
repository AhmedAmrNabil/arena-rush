#include "shader.hpp"

#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Forward definition for error checking functions
std::string checkForShaderCompilationErrors(GLuint shader);
std::string checkForLinkingErrors(GLuint program);

#ifdef __EMSCRIPTEN__
namespace {
    std::string trim(const std::string& value) {
        size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) begin++;

        size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) end--;

        return value.substr(begin, end - begin);
    }

    bool startsWith(const std::string& value, const std::string& prefix) {
        return value.rfind(prefix, 0) == 0;
    }

    void replaceAll(std::string& value, const std::string& from, const std::string& to) {
        size_t position = 0;
        while ((position = value.find(from, position)) != std::string::npos) {
            value.replace(position, from.size(), to);
            position += to.size();
        }
    }

    std::string flattenVaryingBlocks(const std::string& source) {
        std::istringstream input(source);
        std::string result;
        std::string line;

        while (std::getline(input, line)) {
            std::string stripped = trim(line);
            const bool isVaryingOutput =
                startsWith(stripped, "out Varyings") && stripped.find('{') != std::string::npos;
            const bool isVaryingInput = startsWith(stripped, "in Varyings") && stripped.find('{') != std::string::npos;

            if (!isVaryingOutput && !isVaryingInput) {
                result += line;
                result += '\n';
                continue;
            }

            const std::string qualifier = isVaryingOutput ? "out" : "in";
            while (std::getline(input, line)) {
                stripped = trim(line);
                if (stripped.find('}') != std::string::npos) break;

                const size_t semicolon = stripped.find(';');
                if (semicolon == std::string::npos) continue;

                std::istringstream declaration(stripped.substr(0, semicolon));
                std::string type;
                std::string name;
                if (declaration >> type >> name) {
                    result += qualifier + " " + type + " v_" + name + ";\n";
                }
            }
        }

        replaceAll(result, "vs_out.", "v_");
        replaceAll(result, "fs_in.", "v_");
        return result;
    }

    std::string toWebGLShaderSource(std::string source) {
        constexpr const char* webglHeader = "#version 300 es\nprecision highp float;\nprecision highp int;\n";

        if (source.rfind("#version", 0) != 0) {
            return std::string(webglHeader) + flattenVaryingBlocks(source);
        }

        const auto firstLineEnd = source.find('\n');
        if (firstLineEnd == std::string::npos) {
            return webglHeader;
        }

        source.erase(0, firstLineEnd + 1);
        return std::string(webglHeader) + flattenVaryingBlocks(source);
    }
}  // namespace
#endif

bool our::ShaderProgram::attach(const std::string& filename, GLenum type) const {
    // Here, we open the file and read a string from it containing the GLSL code of our shader
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "ERROR: Couldn't open shader file: " << filename << std::endl;
        return false;
    }
    std::string sourceString = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
#ifdef __EMSCRIPTEN__
    sourceString = toWebGLShaderSource(sourceString);
#endif
    const char* sourceCStr = sourceString.c_str();
    file.close();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    std::string error = checkForShaderCompilationErrors(shader);
    if (!error.empty()) {
        std::cerr << "ERROR: Shader compilation failed for shader: " << filename << std::endl;
        std::cerr << error << std::endl;
        glDeleteShader(shader);
        return false;
    }
    glAttachShader(program, shader);
    glDeleteShader(shader);

    return true;
}

bool our::ShaderProgram::link() const {
    uniformLocations.clear();
    glLinkProgram(program);
    std::string error = checkForLinkingErrors(program);
    if (!error.empty()) {
        std::cerr << "ERROR: Shader linking failed." << std::endl;
        std::cerr << error << std::endl;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////
// Function to check for compilation and linking error in shaders //
////////////////////////////////////////////////////////////////////

std::string checkForShaderCompilationErrors(GLuint shader) {
    // Check and return any error in the compilation process
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* logStr = new char[length];
        glGetShaderInfoLog(shader, length, nullptr, logStr);
        std::string errorLog(logStr);
        delete[] logStr;
        return errorLog;
    }
    return std::string();
}

std::string checkForLinkingErrors(GLuint program) {
    // Check and return any error in the linking process
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char* logStr = new char[length];
        glGetProgramInfoLog(program, length, nullptr, logStr);
        std::string error(logStr);
        delete[] logStr;
        return error;
    }
    return std::string();
}
