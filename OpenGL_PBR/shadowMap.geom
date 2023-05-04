#version 460
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos; 

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		gl_Layer = face; //Built in variable to tell OpenGL which face we want to use
		for (int i = 0; i < 3; ++i) //Each vertex of the face
		{
			FragPos = gl_in[i].gl_Position;
			gl_Position = shadowMatrices[face] * FragPos; //Translate world space by passed through light space matrix
			EmitVertex();
		}
		EndPrimitive();
	}
}