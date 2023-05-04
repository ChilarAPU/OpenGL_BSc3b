#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "Texture.h"
#include "Shader.h"

using namespace std;
using namespace glm;

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoords;
};

struct MTexture
{
	unsigned int id;
	string type;
	string path; //store path of texture to compare with other queries of the same texture
	bool operator==(const MTexture& T1)
	{
		return this->path == T1.path;
	}
};

class Mesh
{
public:
	//mesh data
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<MTexture> textures;

	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<MTexture> textures, bool bInstanced);
	//Draw mesh to viewport
	void Draw(Shader& shader, bool bInstanced);

	unsigned int GetVAO();
	vector<unsigned int> GetIndices();
private:
	//render data
	unsigned int VAO, VBO, EBO;

	// Setup OpenGL buffers 
	void setupMesh();

	void setupInstancedMesh();

};

