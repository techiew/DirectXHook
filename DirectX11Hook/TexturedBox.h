#pragma once
#include "Mesh.h"

class TexturedBox : public Mesh
{
public:
	TexturedBox(float posX, float posY, float width, float height, int textureIndex);
};