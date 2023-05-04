#version 460
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;

uniform bool horizontal; // Whether to blur horizontaly this pass or not
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216); //Gaussain values to weight incoming colours

void main()
{
	//Size of a single texel
	vec2 tex_offset = 1.0 / textureSize(image, 0);
	vec3 result = texture(image, TexCoords).rgb * weight[0]; //Current fragment colour along with gaussian weight
	//5 Passes for each direction of the two pass gaussian blur
	if (horizontal) //Blur horizontally
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i]; //Opposite direction
		}
	}
	else //Blur vertically
	{
		for (int i = 1; i < 5; ++i)
		{
			result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i]; //Opposite direction
		}
	}
	FragColor = vec4(result, 1.0);
}