#version 460
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;

layout (std140) uniform Matrices
{
	mat4 projection;  //base allignment of 16 4 times, each with a different alligned offset
};

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  