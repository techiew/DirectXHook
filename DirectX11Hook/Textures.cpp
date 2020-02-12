#include "Textures.h"

using namespace DirectX;
using namespace Microsoft::WRL;

Textures::Textures(ID3D11Device* device, DebugConsole* console)
{
	this->device = device;
	this->console = console;
	textures = std::vector<ComPtr<ID3D11ShaderResourceView>>();
}

int Textures::LoadTexture(std::string filepath)
{
	if (device == nullptr)
	{
		console->PrintDebugMsg("Could not load texture, device was nullptr", nullptr, MsgType::FAILED);
		return -1;
	}

	console->PrintDebugMsg("Loading texture: %s", (void*)filepath.c_str());

	ComPtr<ID3D11ShaderResourceView> texture = nullptr;

	// Convert our filepath string to a wide string, because Windows likes wide characters
	std::wstring wideString(filepath.length(), ' ');
	std::copy(filepath.begin(), filepath.end(), wideString.begin());

	std::fstream file = std::fstream(filepath);

	if (file.fail())
	{
		console->PrintDebugMsg("Texture loading failed, file was not found at: %s", (void*)filepath.c_str(), MsgType::FAILED);
		file.close();
		return -1;
	}

	file.close();

	HRESULT texResult = CreateWICTextureFromFile(device.Get(), wideString.c_str(), nullptr, texture.GetAddressOf());

	_com_error texErr(texResult);
	console->PrintDebugMsg("Texture HRESULT: %s", (void*)texErr.ErrorMessage());

	if (FAILED(texResult))
	{
		console->PrintDebugMsg("Texture loading failed: %s", (void*)filepath.c_str(), MsgType::FAILED);
		return -1;
	}
	
	textures.push_back(texture);

	return textures.size() - 1;
}

ID3D11ShaderResourceView* Textures::Get(int textureIndex)
{
	if (textureIndex < 0 || textureIndex >= textures.size()) return nullptr;
	return textures.at(textureIndex).Get();
}
