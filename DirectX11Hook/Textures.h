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
public:
	Textures(ID3D11Device* device);
	int LoadTexture(std::string filepath);
	ID3D11ShaderResourceView* Get(int textureIndex);

private:
	DebugConsole console;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;
};