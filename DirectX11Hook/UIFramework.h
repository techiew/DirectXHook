#pragma once
#include <vector>
#include <SpriteBatch.h>
#include "UIBox.h"

using namespace DirectX;

class UIFramework
{
public:
	void Box(int width, int height, int x, int y, XMFLOAT3 color);
	void Box(int width, int height, int x, int y, int textureID);
private:
	std::vector<UIBox> boxes = std::vector<UIBox>();
};