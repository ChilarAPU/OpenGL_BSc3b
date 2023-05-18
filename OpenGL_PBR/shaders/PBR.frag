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
	sampler2D ao;
};

uniform Material material;
uniform vec3 viewPos;
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap; //MipMaped environment
uniform sampler2D BRDFLUT; //indirect specular integral

uniform samplerCube shadowMapCube;
uniform float far_plane;

uniform bool bIsTransparent;

//Temp values for PBR testing
//const vec3 albedo = vec3(0.5f, 0.0f, 0.0f);
//const float metallic = 0.5;
//const float roughness = 0.05;
//const float ao = 1.0f;
const float PI = 3.14159265359;

//Calcucate how much light the current fragment should reflect and how much it should refract
vec3 fresnelSchlick(float cosTheta, vec3 F0); 
//Same as fresnelSchlick but inplements a roughness value
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness); 
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness); // Geometry Function
float DistributionGGX(vec3 N, vec3 H, float roughness); //Normal Distribution 
float GeometrySchlickGGX(float NdotV, float roughness);
float ShadowCalculation(vec3 fragPos);

void main()
{
	//texture properties
	vec3 albedo = texture(material.diffuse, fs_in.texCoord).rgb;
	float metallic = texture(material.metallic, fs_in.texCoord).r;
	float roughness = texture(material.roughness, fs_in.texCoord).r;
	vec3 norm = texture(material.normal, fs_in.texCoord).rgb;
	float ao = texture(material.ao, fs_in.texCoord).r;
	//Tranform normal into range [-1, 1]
	//norm = normalize(norm * 2.0 - 1.0);
	norm = norm * 2.0 - 1.0;
	norm = normalize(fs_in.TBN * norm);
	
	//Start of basic PBR
	vec3 N = normalize(norm); //Normal in world space
	//vec3 N = norm;
	vec3 V = normalize(viewPos - fs_in.FragPos); //Direction of fragment to player
	vec3 R = reflect(-V, N);

	//calculate the reflective amount of the current fragment
	vec3 F0 = vec3(0.04); //default F0 value for non-metallic surfaces
	F0 = mix(F0, albedo, metallic);

	//Calculate Point Light Radiance
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 4; i++)
	{
		//per light radiance
		vec3 L = normalize(lightPositions[i] - fs_in.FragPos);
		vec3 H = normalize(V + L);
		float distance = length(lightPositions[i] - fs_in.FragPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		//Cook-Torrance BRDF
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		vec3 kS = F; //specular contribution
		vec3 kD = vec3(1.0) - kS; //diffuse contribution

		kD *= 1.0 - metallic; //More metallic surfaces refract less light

		float NdotL = max(dot(N, L), 0.0);
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	//Shadow calculation
	float shadow = ShadowCalculation(fs_in.FragPos);
	vec3 lightingDif = (1.0 - shadow) * diffuse;

	//emissive calculation
	vec3 emissive = texture(material.emissive, fs_in.texCoord).rgb;
	
	vec3 ambient = (kD * diffuse + specular + emissive) * ao;
	vec3 color = ambient + Lo + lightingDif;

	//Gamma correction on final output
	//color = (color + vec3(1.0));
	//color = pow(color, vec3(1.0/2.2));
	
	FragColor = vec4(color, 1.0);
	//FragColor = vec4(specular.rgb, 1.0);
	//FragColor = vec4(emissive * 5, 1.0);

	//Setup Bloom gate
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BloomColor = FragColor;
    else
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);

	//Transparency
	if (bIsTransparent)
	{
		vec4 texColor = texture(material.opacity, fs_in.texCoord);
		FragColor = texColor;
	}
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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
	vec3 fragToLight = fragPos - lightPositions[0];
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