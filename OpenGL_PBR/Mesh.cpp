#include "Mesh.h"

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<MTexture> textures, bool bInstanced)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	//Initialize variables
	EBO = 0;
	VBO = 0;
	VAO = 0;

	setupMesh();
	//if we are instanced, run the instanced code ontop
	if (bInstanced)
	{
		setupInstancedMesh();
	}
}

void Mesh::Draw(Shader& shader, bool bInstanced)
{
	/* 
		ADJUST THIS! REQUIRES SPECIFIC STRINGS TO BE SET 
		IN ORDER FOR IT TO WORK
	*/
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	unsigned int opacityNr = 1;
	unsigned int metallicNr = 1;
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		//Get current texture unit to bind buffer to
		glActiveTexture(GL_TEXTURE0 + i);

		string number;
		string name = textures[i].type;
		//If we want to use multiple of the same texture type
		if (name == "diffuse")
		{
			number = to_string(diffuseNr++);
		}
		else if (name == "specular")
		{
			number = to_string(specularNr++);
		}
		else if (name == "opacity")
		{
			number = to_string(opacityNr++);
		}
		else if (name == "metallic")
		{
			number = to_string(metallicNr++);
		}

		if (diffuseNr == 1)
		{
			shader.setInt(("material.diffuse"), 0);
		}
		else if (specularNr == 1)
		{
			shader.setInt(("material.specular"), 0);
		}
		else if (opacityNr == 1)
		{
			shader.setInt(("material.opacity"), 0);
		}
		else if (metallicNr == 1)
		{
			shader.setInt(("material.metallic"), 0);
		}

		//Send texture over to fragment shader
		shader.setInt(("material." + name).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
	//reset active texture ready for next call
	glActiveTexture(GL_TEXTURE0);

	if (!bInstanced)
	{
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	else
	{
		shader.setBool("bInstance", true);
		glBindVertexArray(VAO);
		glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, 100);
		glBindVertexArray(0);
		shader.setBool("bInstance", false);
	}
}

unsigned int Mesh::GetVAO()
{
	return this->VAO;
}

vector<unsigned int> Mesh::GetIndices()
{
	return indices;
}

void Mesh::setupMesh()
{
	//Generate VAO, VBO and EBO ready for buffer data
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	//Bind vertices to VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	/* Fill buffer after initialization
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);
	*/

	/* Manually copying data into buffer
	//pointer to writable buffer
	void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	//copy data into buffer
	memcpy(ptr, &vertices[0], vertices.size() * sizeof(Vertex));
	//unmap buffer
	glUnmapBuffer(GL_ARRAY_BUFFER);
	*/

	//Bind indices to EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	//Bind data buffer to OpenGL
	//vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	//vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

	//vertex texture coordinates
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

	//Unbind VAO to stop accidental calls to buffer
	glBindVertexArray(0);

}

void Mesh::setupInstancedMesh()
{
	//Temporary way to store the instanced model matrices until a custom instanced class is made
	vector<vec3> floorVerts;
	vector<mat4> floorModelMats;
	//Temp way to initialize floor instance locations
	int floorWidth = 10; //How wide the floor should be 
	float vertValue = 1.0; //To place floor left
	float HoriValue = 1.5; //To place floor behind
	floorVerts.emplace_back(vec3(5.0, -1.8, 8)); //push starting value
	for (int i = 0; i < 100; i++) //Create 50 floors
	{
		float mod = i % floorWidth; //Have we reached maximum width
		if (mod != 0)
		{
			//Add floor horizontally
			vec3 prev = floorVerts.at(i); //access previous value
			vec3 newVert = prev;
			newVert.x -= vertValue;
			floorVerts.push_back(newVert);
			//trying with model matrix instead
			mat4 model = mat4(1.0);
			model = translate(model, newVert);
			model = scale(model, vec3(0.3, 0.3, 0.3));
			floorModelMats.push_back(model);
		}
		else
		{
			// Add Floor vertically
			vec3 prev = floorVerts.at(i); //access previous value
			vec3 newVert = prev;
			newVert.x = floorVerts.at(0).x; //TEMP WAY TO REVERT HORIZONAL VALUE
			newVert.z -= HoriValue;
			floorVerts.push_back(newVert);
			//trying with model matrix instead
			mat4 model = mat4(1.0);
			model = translate(model, newVert);
			model = scale(model, vec3(0.3, 0.3, 0.3));
			floorModelMats.push_back(model);
		}
	}

	/*unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, floorModelMats.size() * sizeof(mat4), &floorModelMats.at(0), GL_STATIC_DRAW);

	//Temp way to pass through vertices to vertex shader
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)0);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(sizeof(vec4)));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(2 * sizeof(vec4)));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(3 * sizeof(vec4)));

	//Tell OpenGL that we want to update the value every instance
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	glBindVertexArray(0);
	*/

	GLuint ssboModelMatrices;
	glGenBuffers(1, &ssboModelMatrices);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboModelMatrices);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(mat4) * floorModelMats.size(), &floorModelMats.at(0), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboModelMatrices); //Allocate shader storage buffer to binding point 1
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
