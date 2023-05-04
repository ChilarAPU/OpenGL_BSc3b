#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Shader
{
private:

	void ReadSourceFile(string& vertexFile, string& fragmentFile, string& geometryFile, const char* vertexPath, const char* fragmentPath, const char* geometryPath);

	void CompileShaders(const char* vertexCode, const char* fragmentCode, const char* geometryCode);

public:
	unsigned int ID;

	//Default constructor 
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);

	//Use/activate this shader class
	void use();

	//uniform query functions
	void setBool(const string& name, bool value) const;
	void setInt(const string& name, int value) const;
	void setFloat(const string& name, float value) const;
	void setMat4(const string& name, mat4& matrix) const;
	void setVec3(const string& name, vec3 value) const;
};