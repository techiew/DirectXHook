#pragma once

#include <vector>
#include <d3d11.h>
#include <WICTextureLoader.h>
#include <comdef.h>
#include <fstream>
#include <wrl/client.h>
#include "DebugConsole.h"

class Textures
{
private:
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	DebugConsole* console;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;

public:
	Textures() {};
	Textures(ID3D11Device* device, DebugConsole* console);
	int LoadTexture(std::string filepath);
	ID3D11ShaderResourceView* Get(int textureIndex);
};