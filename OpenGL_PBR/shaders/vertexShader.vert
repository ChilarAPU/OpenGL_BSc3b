#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
//layout (location = 3) in mat4 instanceMatrix; //Used with instancing, takes up slot 3, 4, 5 and 6
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

uniform mat4 model;
uniform mat4 lightSpaceMatrix; //Depth value from shadow map
//uniform mat4 view;
//uniform mat4 projection;
//uniform vec3 lightPos;
//uniform vec3 viewPos;

layout (std140) uniform Matrices
{
	mat4 projection;  //base allignment of 16 4 times, each with a different alligned offset
	mat4 view;
};

layout (std430, binding = 1) buffer ModelMatrices
{
	mat4 modelMatrix[];
};

//Group up all output values inside an interface
out VS_OUT
{
	vec2 texCoord;
	vec3 Normal;
	vec3 FragPos; //Position of fragment in world space
	vec4 FragPosLightSpace; //Vertex position in light space
	//Transforming world space to tangent space for normals
	//vec3 TangentLightPos;
	//vec3 TangentViewPos;
	//vec3 TangentFragPos;
	mat3 TBN;
} vs_out;

uniform bool bInstance = false;

void main()
{
	if (!bInstance)
	{
		gl_Position = projection * view * model * vec4(aPos, 1.0f);
		vs_out.Normal = mat3(transpose(inverse(model))) * aNormal; //this calculation deals with non-uniform scaling 
		vs_out.FragPos = vec3(model * vec4(aPos, 1.0)); //Position value in world space coordinates that can be used by the fragment shader
	}
	else
	{
		gl_Position = projection * view * modelMatrix[gl_InstanceID] * vec4(aPos, 1.0f);
		vs_out.Normal = mat3(transpose(inverse(modelMatrix[gl_InstanceID]))) * aNormal; //this calculation deals with non-uniform scaling 
		vs_out.FragPos = vec3(modelMatrix[gl_InstanceID] * vec4(aPos, 1.0)); //Position value in world space coordinates that can be used by the fragment shader
	}
	//Transforming light and view positions into tangent space
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * aNormal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	vs_out.TBN = transpose(mat3(T, B, N));
	//vs_out.TangentLightPos = TBN * lightPos;
	//vs_out.TangentViewPos = TBN * viewPos;
	//vs_out.TangentFragPos = TBN * vs_out.FragPos;

	//vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0); //Vertex Position from the direction of the light source
	vs_out.texCoord = aTexCoord;
	//mulitply position of incoming vertex by the model matrix
	//Increase point size the further away the camera is
	gl_PointSize = gl_Position.z;
}