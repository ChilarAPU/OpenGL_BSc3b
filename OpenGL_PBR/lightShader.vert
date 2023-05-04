#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 vertexColor;
out vec2 texCoord;

uniform mat4 model;

layout (std140) uniform Matrices
{
	mat4 projection;  //base allignment of 16 4 times, each with a different alligned offset
	mat4 view;
};

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vertexColor =  aColor;
	texCoord = aTexCoord;
}
