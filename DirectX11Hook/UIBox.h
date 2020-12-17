#pragma once

#include <SpriteBatch.h>

using namespace DirectX;

class UIBox
{
public:
	int width;
	int height;
	int x, y;
	bool visible = true;
	bool clicked = false;

private:
	XMFLOAT3 color;
	int textureID;
};