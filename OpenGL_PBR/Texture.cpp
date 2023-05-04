
#include <STB/stb_image.h>
#include "Texture.h"



Texture::Texture()
{
	ID = 0;
	type = "";
}

string Texture::GetName()
{
	return type;
}

unsigned int Texture::GetID()
{
	return ID;
}

void Texture::LoadTexture(const char* path)
{
	ID = 0;
	//tell stbi to flip incoming images on load
	stbi_set_flip_vertically_on_load(false);

	//Load texture from file using stb
	int width, height, nrChannels;

	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

	if (data)
	{
		//Generate texture into shader 
		glGenTextures(1, &ID); //Generate 1 texture buffer
		glBindTexture(GL_TEXTURE_2D, ID);

		GLenum format;

		switch (nrChannels)
		{
		case 1:
			format = GL_RED;
			break;
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D); //Automatically generate mipmap for passed through texture
		//How should we handle texture coordinates outside of 0-1
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//Texture filtering for both scalling up and down
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//Free memory that holds the source texture data
		// Have to call free() as stbi_image_free() crashes with .jpg files
		//stbi_image_free(data);
		free(data);

	}
	else
	{
		cout << "FAILED TO LOAD TEXTURE: " << path << endl;
		//Free memory that holds the source texture data
		stbi_image_free(data);
	}
}

void Texture::BindTextureToBuffer(GLenum slot)
{
	glActiveTexture(slot);
	glBindTexture(GL_TEXTURE_2D, ID);
}
