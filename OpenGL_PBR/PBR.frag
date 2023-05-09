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
	sampler2D roughness;
	sampler2D emissive;
	sampler2D opacity;
	sampler2D metallic;
	sampler2D normal;
	float shininess;
};

uniform Material material;
uniform vec3 viewPos;
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

//Temp values for PBR testing
//const vec3 albedo = vec3(0.5f, 0.0f, 0.0f);
//const float metallic = 0.5;
//const float roughness = 0.05;
const float ao = 1.0f;
const float PI = 3.14159265359;

//Calcucate how much light the current fragment should reflect and how much it should refract
vec3 fresnelSchlick(float cosTheta, vec3 F0); 
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness); // Geometry Function
float DistributionGGX(vec3 N, vec3 H, float roughness); //Normal Distribution 
float GeometrySchlickGGX(float NdotV, float roughness);

void main()
{
	vec3 albedo = texture(material.diffuse, fs_in.texCoord).rgb;
	//vec3 albedo = pow(texture(material.diffuse, fs_in.texCoord).rgb, vec3(2.2));
	float metallic = texture(material.metallic, fs_in.texCoord).r;
	float roughness = texture(material.roughness, fs_in.texCoord).r;
	//Obtain fragment normal from normal map
	vec3 norm = texture(material.normal, fs_in.texCoord).rgb;
	//Tranform normal into range [-1, 1]
	//norm = normalize(norm * 2.0 - 1.0);
	norm = norm * 2.0 - 1.0;
	norm = normalize(fs_in.TBN * norm);
	
	//Start of basic PBR
	vec3 N = normalize(fs_in.Normal); //Normal in world space
	//vec3 N = norm;
	vec3 V = normalize(viewPos - fs_in.FragPos); //Direction of fragment to player

	//Calculate Point Light Radiance
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 4; i++)
	{
		vec3 L = normalize(lightPositions[i] - fs_in.FragPos);
		vec3 H = normalize(V + L);

		float distance = length(lightPositions[i] - fs_in.FragPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		vec3 F0 = vec3(0.04); //default F0 value for non-metallic surfaces
		F0 = mix(F0, albedo, metallic);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);

		//Cook-Torrance BRDF
		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator + denominator;

		vec3 kS = F; //specular contribution
		vec3 kD = vec3(1.0) - kS; //diffuse contribution

		kD *= 1.0 - metallic; //More metallic surfaces refract less light

		float NdotL = max(dot(N, L), 0.0);
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient + Lo;

	//Gamma correction on final output
	//color = (color + vec3(1.0));
	//color = pow(color, vec3(1.0/2.2));
	
	//FragColor = texture(material.diffuse, fs_in.texCoord);
	FragColor = vec4(color, 1.0);

	//Setup Bloom gate
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BloomColor = FragColor;
    else
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness*roughness; //Addition by Epic Games for better results
	float a2 = a*a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0; //Addition by Epic Games for better results

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}