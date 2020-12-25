#pragma once
#include <vector>
#include <SpriteBatch.h>
#include "UIBox.h"

class UIFramework
{
public:
	void Box(int width, int height, int x, int y, DirectX::XMFLOAT3 color);
	void Box(int width, int height, int x, int y, int textureID);
private:
	std::vector<UIBox> boxes = std::vector<UIBox>();
};