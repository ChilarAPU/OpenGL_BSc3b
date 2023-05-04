#version 460

in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
	// get distance between fragment pos and light source
	float lightDistance = length(FragPos.xyz - lightPos);

	//Map to [0,1] range 
	lightDistance = lightDistance / far_plane;

	//Modify depth value
	gl_FragDepth = lightDistance;

}