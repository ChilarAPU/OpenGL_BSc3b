#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;

layout (std140) uniform Matrices
{
	mat4 projection;
	mat4 view;
};

out GS_OUT { //Values passed through to geometry Shader
	vec3 normal;
	mat4 projection;
} gm_out;

void main()
{
	gl_Position = view * model * vec4(aPos, 1.0);
	mat3 normalMatrix = mat3(transpose(inverse(view * model))); //Accomodate for scaling and rotation to the normal vertex
	gm_out.normal = normalize(vec3(vec4(normalMatrix * aNormal, 0.0)));
	gm_out.projection = projection; //pass through projection matrix to geometry shader
}