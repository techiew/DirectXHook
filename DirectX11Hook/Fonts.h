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
	Fonts() {};
	Fonts(ID3D11Device* device, DebugConsole* console);
	int LoadFont(std::string filepath);
	DirectX::SpriteFont* Get(int fontIndex);

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	DebugConsole* console;
	std::vector<std::shared_ptr<DirectX::SpriteFont>> fonts;
};