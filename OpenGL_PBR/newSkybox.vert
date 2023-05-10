#version 460
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 view;

layout (std140) uniform Matrices
{
	mat4 projection;  //base allignment of 16 4 times, each with a different alligned offset
};

void main()
{
    localPos = aPos;

    vec4 clipPos = projection * view * vec4(localPos, 1.0);
    gl_Position = clipPos.xyww;
}  