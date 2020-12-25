#pragma once

#include <vector>
#include <d3d11.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <comdef.h>
#include <wrl/client.h>
#include <fstream>
#include <SpriteFont.h>
#include <SpriteBatch.h>
#include "DebugConsole.h"

class Fonts
{
public:
	Fonts(ID3D11Device* device);
	int LoadFont(std::string filepath);
	DirectX::SpriteFont* Get(int fontIndex);

private:
	DebugConsole console;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	std::vector<std::shared_ptr<DirectX::SpriteFont>> fonts;
};