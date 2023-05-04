#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <STB/stb_image.h>
#include <algorithm>
#include "Model.h"

using namespace Assimp;

Model::Model()
	: bIsInstanced(false)
{
}

Model::Model(const char* path)
{
	loadModel(path);
}

void Model::Draw(Shader& shader, int meshToDraw, bool bInstanced)
{
	//Loop over each mesh in the model and render it to the screen
	if (meshToDraw < meshes.size())
	{
		meshes[meshToDraw].Draw(shader, bInstanced);
		return;
	}
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		meshes[i].Draw(shader, bInstanced);
	}

}

void Model::setDiffuseDirectory(const string& directory)
{
	this->diffuseDirectory = directory;
}

void Model::setSpecularDirectory(const string& directory)
{
	this->specularDirectory = directory;
}

void Model::setOpacityDirectory(const string& directory)
{
	this->opacityDirectory = directory;
}

void Model::setMetallicDirectory(const string& directory)
{
	this->metallicDirectory = directory;
}

unsigned int Model::GetVAO()
{
	return meshes.at(0).GetVAO();
}

vector<unsigned int> Model::GetIndices()
{
	return meshes.at(0).GetIndices();
}

void Model::loadModel(string path)
{
	Importer importer;
	//Force model to load as triangles 
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	//Make sure the passed through model has been read correctly
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERRROR::ASSIMP::" << importer.GetErrorString() << endl;
		return;
	}
	directory = path.substr(0, path.find_last_of("/"));

	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	//process all nodes meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		//remove mesh from array once it has been processed
		meshes.push_back(processMesh(mesh, scene));
	}
	//go to next child
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<MTexture> textures;

	//setup vertices 
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		//get position of vertex
		vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;
		
		//get normals of vertices
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;

		//get texture coordinates of vertex
		if (mesh->mTextureCoords[0]) //Does the imported mesh contain texture coordinates
		{
			vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		} 
		else
		{
			vertex.TexCoords = vec2(0.0, 0.0);
		}

		vertices.push_back(vertex);
	}

	//setup indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) //As triangulate is set, there should be 3 indices per face
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	//Setup materials
	/*if (mesh->mMaterialIndex >= 0) //Does the imported mesh contain material indexes
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//load diffuse textures
		vector<MTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end()); //insert all diffuse maps into texture vector

		//load specular maps
		vector<MTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end()); //insert all specular maps into texture vector
	}
	*/

	//Setup texture directly, ignoring the .mtl file

	//diffuse
	TextureLoadReturn texture = loadTexture(aiTextureType_DIFFUSE, diffuseDirectory, "diffuse");
	if (texture.bIsNewTexture)
	{
		textures.push_back(texture.texture);
	}
	//specular
	if (!specularDirectory.empty())
	{
		texture = loadTexture(aiTextureType_SPECULAR, specularDirectory, "specular");
			if (texture.bIsNewTexture)
			{
			textures.push_back(texture.texture);
			}
	}
	//opacity
	if (!opacityDirectory.empty())
	{
		texture = loadTexture(aiTextureType_OPACITY, opacityDirectory, "opacity");
		if (texture.bIsNewTexture)
		{
			textures.push_back(texture.texture);
		}
	}
	//metallic
	if (!metallicDirectory.empty())
	{
		texture = loadTexture(aiTextureType_METALNESS, metallicDirectory, "metallic");
		if (texture.bIsNewTexture)
		{
			textures.push_back(texture.texture);
		}
	}
	
	return Mesh(vertices, indices, textures, bIsInstanced);
}

vector<MTexture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
	vector<MTexture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) //Loop through each texture of sepcified type
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++)
		{
			if (strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) //Compare current texture path to all currently loaded textures
			{
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{
			//If the texture has not been loaded before
			MTexture texture;
			texture.id = TextureFromFile(str.C_Str(), directory); //load texture
			texture.type = typeName; //set texture name for use with fragment shader
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture); //add to loaded textures
		}
		
	}

	return textures;
}

TextureLoadReturn Model::loadTexture(aiTextureType type, string pathToTexture, string typeName)
{
	//vector<MTexture> textures;
	TextureLoadReturn texture;
	texture.bIsNewTexture = true;

	//MTexture texture;
	//Check to make sure we have not already loaded the texture
	texture.texture.path = pathToTexture;
	bool b = any_of(textures_loaded.begin(), textures_loaded.end(), [texture](MTexture x) { return texture.texture.path == x.path; });
	//have we already loaded this texture before
	if (b)
	{
		//if we have, return nothing
		texture.bIsNewTexture = false;
		return texture;
	}
	//load texture to textures_loaded
	texture.texture.id = TextureFromFile(pathToTexture.c_str(), directory);
	texture.texture.type = typeName;
	textures_loaded.push_back(texture.texture);

	return texture;
}

unsigned int Model::TextureFromFile(const char* path, const string& directory, bool gamma)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	stbi_set_flip_vertically_on_load(false);

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			//format = GL_SRGB_ALPHA; //Specifying we are using sRGB textures. Only do this for diffuse otherwise lighting will have undesirable results
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		free(data);

		glBindTexture(GL_TEXTURE_2D, 0);

		cout << filename << " loaded" << endl;
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		cout << stbi_failure_reason() << endl;
		free(data);
	}

	return textureID;
}
