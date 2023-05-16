#version 460
out vec4 FragColor;

in vec3 localPos; //Local position that acts as face direction

uniform samplerCube skyboxMap;

void main()
{    
    //vec3 envColor = textureLod(skyboxMap, localPos, 1.2).rgb;
    FragColor = texture(skyboxMap, localPos);
}