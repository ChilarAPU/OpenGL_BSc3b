#version 460
out vec4 FragColor;

in vec3 localPos; //Local position that acts as face direction

uniform samplerCube skyboxMap;

void main()
{    
    FragColor = texture(skyboxMap, localPos);
    //vec3 envColor = texture(skyboxMap, localPos).rgb;

}