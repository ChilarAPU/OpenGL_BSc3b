#version 460
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

in vec3 vertexColor;
in vec2 texCoord;

uniform vec3 lightColor;

void main()
{
	FragColor = vec4(lightColor, 1.0);
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.15)
        BloomColor = FragColor;
    else
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
}