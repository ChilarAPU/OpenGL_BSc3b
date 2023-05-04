#pragma once
#include <assimp/scene.h>
#include <vector>

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

struct TextureLoadReturn
{
	MTexture texture;
	bool bIsNewTexture;
};

class Model
{
public:

	/* Must load textures before calling loadModel() */
	Model();

	Model(const char* path);
	/* meshToDraw is to be used when only a certain mesh from a model wants to be drawn. Leave as default to render every mesh. 
	If the value given is too high, then default behaviour of drawing the whole mesh is used*/
	void Draw(Shader& shader, int meshToDraw = -1, bool bInstanced = false);

	void loadModel(string path);

	void setDiffuseDirectory(const string& directory);
	void setSpecularDirectory(const string& directory);
	void setOpacityDirectory(const string& directory);
	void setMetallicDirectory(const string& directory);

	bool bIsInstanced;

	unsigned int GetVAO();
	vector<unsigned int> GetIndices();

private:
	//model data
	vector<Mesh> meshes;
	vector<MTexture> textures_loaded; //stores loaded textures 
	string directory;

	string diffuseDirectory;
	string specularDirectory;
	string opacityDirectory;
	string metallicDirectory;

	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<MTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

	TextureLoadReturn loadTexture(aiTextureType type, string pathToTexture, string typeName);

	unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);
};

