#pragma once
#include <vector>
#include <d3d11.h>
#include "DebugConsole.h"
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <comdef.h>
#include <wrl/client.h>
#include <fstream>
#include <SpriteFont.h>
#include <SpriteBatch.h>

class Fonts
{
private:
	DebugConsole* console;
	std::vector<std::shared_ptr<DirectX::SpriteFont>> fonts;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;

public:
	Fonts() { };
	Fonts(DebugConsole* console);
	void SetDevice(ID3D11Device* device);
	int LoadFont(std::string filepath);
	DirectX::SpriteFont* GetFont(int fontIndex);
};