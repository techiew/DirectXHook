#pragma once

#include <SpriteBatch.h>

class UIBox
{
public:
	int width;
	int height;
	int x, y;
	bool visible = true;
	bool clicked = false;

private:
	DirectX::XMFLOAT3 color;
	int textureID;
};