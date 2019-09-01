#pragma once
#include <vector>
#include <d3d11.h>
#include <DDSTextureLoader.h>
#include "DebugConsole.h"
#include <comdef.h>
#include <wrl/client.h>

class Textures
{
private:
	DebugConsole* console;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures; // Textures are ordered from first to last loaded (0 to current size)
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;

public:
	Textures() {};
	Textures(DebugConsole* console);
	void SetDevice(ID3D11Device* device);
	int LoadTexture(std::string filepath);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture(int textureIndex);
	~Textures() {};
};