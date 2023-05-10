#version 460
out vec4 FragColor;

in vec3 TexCoords; //Local position that acts as face direction

uniform samplerCube skybox;

uniform sampler2D HDRImap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v); //Spherical to cartesian trigonometry 

void main()
{    
    //FragColor = texture(skybox, TexCoords);

    vec2 uv = SampleSphericalMap(normalize(TexCoords));
    vec3 colour = texture(HDRImap, uv).rgb;

    FragColor = vec4(colour, 1.0);
}

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}