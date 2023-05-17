#version 460
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in GS_OUT
{
	vec3 normal;
	mat4 projection;
} gm_in[];

const float MAGNITUDE = 0.1;


//in VS_OUT
//{
	//vec2 texCoord;
	//vec3 Normal;
	//vec3 FragPos; //Position of fragment in world space
//} gm_in[];

//out VS_OUT
//{
	//vec2 texCoord;
	//vec3 Normal;
	//vec3 FragPos;
//} fs_out;

vec3 GetNormal();
vec4 explode(vec4 position, vec3 normal);
void GenerateLine(int index);

void main()
{
	vec3 normal = GetNormal(); //Get the normal vector for the incoming primitive

	//gl_Position = gl_in[0].gl_Position;
	//gl_Position = explode(gl_in[0].gl_Position, normal);
	//fs_out.Normal = gm_in[0].Normal;
	//fs_out.texCoord = gm_in[0].texCoord;
	//fs_out.FragPos = gm_in[0].FragPos;
	//EmitVertex();

	//gl_Position = gl_in[1].gl_Position;
	//gl_Position = explode(gl_in[1].gl_Position, normal);
	//fs_out.Normal = gm_in[1].Normal;
	//fs_out.texCoord = gm_in[1].texCoord;
	//fs_out.FragPos = gm_in[1].FragPos;
	//EmitVertex();

	//gl_Position = gl_in[2].gl_Position;
	//gl_Position = explode(gl_in[2].gl_Position, normal);
	//fs_out.Normal = gm_in[2].Normal;
	//fs_out.texCoord = gm_in[2].texCoord;
	//fs_out.FragPos = gm_in[2].FragPos;
	//EmitVertex();

	//EndPrimitive();

	//Generate a line for each vertex of the triangle
	GenerateLine(0);
	GenerateLine(1);
	GenerateLine(2);
}

void GenerateLine(int index)
{
	gl_Position = gm_in[index].projection * gl_in[index].gl_Position; //First point set to current vertex position
	EmitVertex();
	//Second point set to move in the direction of the normal vector at the length of MAGNITUDE
	gl_Position = gm_in[index].projection * (gl_in[index].gl_Position + vec4(gm_in[index].normal, 0.0) * MAGNITUDE); 
	EmitVertex();
	EndPrimitive();
}

vec3 GetNormal()
{
	//Use world space coordinate for normal to negate camera space transformations
	//Without this, faces move and turn along with the camera
	//vec3 a = vec3(gm_in[0].FragPos) - vec3(gm_in[1].FragPos);
	//vec3 b = vec3(gm_in[2].FragPos) - vec3(gm_in[1].FragPos);
	//return normalize(cross(a, b));
	return vec3(1.0);
}

vec4 explode(vec4 position, vec3 normal)
{
	float magnitude = 0;
	vec3 direction = normal * magnitude;
	return position + vec4(direction.xy, 0.0, 0.0);
}