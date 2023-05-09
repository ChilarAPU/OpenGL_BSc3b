#version 460
//Used with multiple color attachments in the incoming framebuffer
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

//Group up all input values into an interface
in VS_OUT
{
	vec2 texCoord;
	vec3 Normal;
	vec3 FragPos;
	vec4 FragPosLightSpace;
	//vec3 TangentLightPos;
	//vec3 TangentViewPos;
	//vec3 TangentFragPos;
	mat3 TBN;
} fs_in;

struct Material
{
	sampler2D diffuse; 
	sampler2D specular;
	sampler2D emissive;
	sampler2D opacity;
	sampler2D metallic;
	sampler2D normal;
	float shininess;
};

struct DirLight
{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight
{
	vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;

	float cutOff;
	float outerCutOff;

	vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight dirLight;
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

uniform Material material;
uniform vec3 viewPos;

uniform sampler2D grassTexture; 
uniform bool bIsTransparent;
uniform sampler2D shadowMap; 

uniform samplerCube shadowMapCube; 
uniform float far_plane;

uniform vec3 lightPos;

//get skybox cubemap for basic reflections
uniform samplerCube skybox;

//uniform sampler2D texture_diffuse1;

//Mediate performance penalty from writing to gl_FragDepth
layout (depth_greater) out float gl_FragDepth;

// Give attenuation a default value as not every light caster uses it
float ShadowCalculation(vec4 fragPosLightSpace);
float ShadowCalculation(vec3 fragPos);
vec3 calculatePhongLighting(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 ambientColor, vec3 diffuseColor, vec3 specularColor, float attenuation = 1);
vec3 calculateDirectionalLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{

	//Get normalized normal and direction of fragment from camera
	//vec3 norm = normalize(fs_in.Normal);
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec4 currentValue;

	//Obtain fragment normal from normal map
	vec3 norm = texture(material.normal, fs_in.texCoord).rgb;
	//Tranform normal into range [-1, 1]
	//norm = normalize(norm * 2.0 - 1.0);
	norm = norm * 2.0 - 1.0;
	norm = normalize(fs_in.TBN * norm);

	//vec3 viewDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);

	//vec3 ambient;
	//vec3 diffuse;
	//vec3 specular;

	//Directional light
	vec3 result = calculateDirectionalLight(dirLight, norm, viewDir);
	//Point lights
	for (int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		result += calculatePointLight(pointLights[i], norm, fs_in.FragPos, viewDir);
	}
	//SpotLights
	//result += calculateSpotLight(spotLight, norm, fs_in.FragPos, viewDir);
	//FragColor = vec4(result, 1.0);
	if (!bIsTransparent)
	{
		FragColor = vec4(result, 1.0);
	}
	else
	{
		vec4 texColor = texture(material.opacity, fs_in.texCoord);
		//if (texColor.a < 0.1)
		//{
			//discard;
		//}

		FragColor = texColor;
	}

	//Basic reflection formula
	vec3 metallicAmount = vec3(texture(material.metallic, fs_in.texCoord).r);
	float ratio = 1.00/ 2.42;
	vec3 I = normalize(fs_in.FragPos - viewPos);
	//vec3 R = reflect(I, norm);
	vec3 R = refract(I, norm, ratio);
	vec3 metallicFinal = R * metallicAmount;
	//FragColor = vec4(texture(skybox, metallicFinal).rgb, 1.0);
	//FragColor = vec4(metallicFinal, 1.0);
	
	//Transform depth testing back to linear curve
	//float ndc = gl_FragCoord.z * 2.0 - 1.0;
	//float linearDepth = (2.0 * 0.1 * 100) / (100 + 0.1 - ndc * (100 - 0.1));
	//float depth = linearDepth / 50.0;

	//FragColor = vec4(vec3(depth), 1.0);

	//Different FragColor value for left hand side of the screen
	//if (gl_FragCoord.x < 600)
	//{
		//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	//}

	//FragColor = vec4(vec3(texture(material.diffuse, fs_in.texCoord)), 1.0);

	//Adjust FragColor based on whether face is front or back facing.
	if (!gl_FrontFacing)
	{
	//Faces inside a model will display a green color as opposed to the normal texture
		FragColor = vec4(0.0, 1.0, 0.0, 1.0);
	}

	//Gives the effect of view frustum being much closer than it actually is
	//gl_FragDepth = gl_FragCoord.z + 0.05;
	float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BloomColor = FragColor;
    else
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
}

vec3 calculatePhongLighting(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 ambientColor, vec3 diffuseColor, vec3 specularColor, float attenuation = 1)
{
	//diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	//specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, lightDir), 0.0), material.shininess);
	//combine diffuse, specular and ambient
	vec3 ambient = ambientColor * vec3(texture(material.diffuse, fs_in.texCoord));
	vec3 diffuse = diffuseColor * diff * vec3(texture(material.diffuse, fs_in.texCoord));
	vec3 specular = specularColor * spec * vec3(texture(material.specular, fs_in.texCoord));
	//multiply lighting by any passed through attenuation
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	//calculate ShadowCalculation
	float shadow = ShadowCalculation(fs_in.FragPos);
	vec3 lightingDif = (1.0 - shadow) * diffuse;

	return (ambient + lightingDif + specular);
}

vec3 calculateDirectionalLight(DirLight light, vec3 normal, vec3 viewDir)
{
	//vec3 lightDir = normalize(-light.direction); //normalize and invert incoming light direction
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	//vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	
	return calculatePhongLighting(lightDir, normal, viewDir, light.ambient, light.diffuse, light.specular);
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos); //Get direction based from position of light source and fragment position
	//vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir); //get halfway vector to blinn-phong

	//attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (1.0f + 0.09f * distance + 0.032f * (distance * distance));

	return calculatePhongLighting(halfwayDir, normal, viewDir, light.ambient, light.diffuse, light.specular, attenuation);
}


vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);

	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (1.0f + 0.09f * distance + 0.032f * (distance * distance));

	//Radius light source from source of spot light and gradually decrease from cutOff to outerCutOff
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

	//emissive
	vec3 emissive = vec3(0.0f, 0.0f, 0.0f);
	//if our specular map value is higher than 0, this means we are on a fragment with the wooden texture
	if (texture(material.specular, fs_in.texCoord).rgb == 0) 
	{
		emissive = texture(material.emissive, fs_in.texCoord).rgb;
	}
	//Use attenuation and spotlight intensity with emissive texture to convey the look of a hidden image on the mesh
	emissive *= attenuation * intensity * 2;

	vec3 phongLighting = calculatePhongLighting(lightDir, normal, viewDir, light.ambient, light.diffuse, light.specular, attenuation);

	//Multiply phong lighting by the spotlight 
	phongLighting *= intensity;

	return (phongLighting + emissive);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
	//perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	//Transform to the range of [0,1]
	projCoords = projCoords * 0.5 + 0.5;
	//Checking if we are outside the frustum of the shadow map
	if (projCoords.z > 1.0)
	{
		return 0.0f;
	}
	//get cloest depth value from lights perspective
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	//get depth of current fragment from lights perspective
	float currentDepth = projCoords.z;
	//Check whether we are the closest to the light source depth value
	//Implement bias to handle shadow acne at angles
	float bias = max(0.05 * (1.0 - dot(fs_in.Normal, lightPos)), 0.005);
	//float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	float shadow = 0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	return shadow;
}

float ShadowCalculation(vec3 fragPos)
{
	//Perpendicular directions of the cubemap to reduce redundant calls
	vec3 sampleOffsetDirections[20] = vec3[]
	(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
	);   
	vec3 fragToLight = fragPos - lightPos;
	float currentDepth = length(fragToLight);
	float viewDistance = length(viewPos - fragPos);
	//Make shadows sharper when player is close to shadow
	float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;

	//Same PCF filter but for cubemaps
	float shadow = 0.0f;
	float bias = 0.15;
	float samples = 20.0f; //pass through size of the array
	float offset = 0.1f;
	for (int i = 0; i < samples; i++)
	{
		float closestDepth = texture(shadowMapCube, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= far_plane;
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);


	return shadow;
}