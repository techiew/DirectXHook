#include "Textures.h"

using namespace DirectX;
using namespace Microsoft::WRL;

Textures::Textures(DebugConsole* console)
{
	this->console = console;
	textures = std::vector<ComPtr<ID3D11ShaderResourceView>>();
}

void Textures::SetDevice(ID3D11Device* device)
{
	this->device = device;
}

int Textures::LoadTexture(std::string filepath)
{
	if (device == nullptr) return -1;

	console->PrintDebugMsg("Loading texture: %s", (void*)filepath.c_str(), MsgType::PROGRESS);

	ComPtr<ID3D11ShaderResourceView> texture = nullptr;

	// Convert our filepath string to a wide string, because Windows likes wide characters
	std::wstring wideString(filepath.length(), ' ');
	std::copy(filepath.begin(), filepath.end(), wideString.begin());

	HRESULT texResult = CreateDDSTextureFromFile(device.Get(), wideString.c_str(), nullptr, texture.GetAddressOf());
	_com_error texErr(texResult);
	console->PrintDebugMsg("Texture HRESULT: %s", (void*)texErr.ErrorMessage(), MsgType::PROGRESS);

	if (FAILED(texResult))
	{
		console->PrintDebugMsg("Texture loading failed: %s - invalid .DDS format?", (void*)filepath.c_str(), MsgType::FAILED);
		textures.push_back(nullptr);
		return -1;
	}
	
	textures.push_back(texture);

	return textures.size() - 1;
}

ComPtr<ID3D11ShaderResourceView> Textures::GetTexture(int textureIndex)
{
	if (textureIndex == -1 || textureIndex >= textures.size()) return nullptr;
	return textures.at(textureIndex);
}
