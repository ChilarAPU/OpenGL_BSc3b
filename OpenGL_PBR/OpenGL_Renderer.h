#pragma once

#include "Shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader;

class OpenGL_Renderer
{
private:
	unique_ptr<GLFWwindow> window;
public:
	void setupGLFWWindow();
};

