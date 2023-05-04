#pragma once

#include <iostream>
#include <glad/glad.h>

using namespace std;

class Texture
{
private:
	unsigned int ID;

	string type;

public:
	/* load texture image into ID variable*/
	Texture();

	string GetName();

	unsigned int GetID();

	void LoadTexture(const char* path);

	void BindTextureToBuffer(GLenum slot);
};

