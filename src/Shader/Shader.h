#pragma once
#include <glad/glad.h>
class Shader {
	void checkCompileErrors(unsigned int shader, GLenum type);

public:
		unsigned int ID;
		Shader();
		~Shader();
		Shader& addShader(const char* path, GLenum type);
		void link();
		void use();
};