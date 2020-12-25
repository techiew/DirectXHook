#include "Fonts.h"

using namespace DirectX;

Fonts::Fonts(ID3D11Device* device)
{
	console = DebugConsole();
	this->device = device;
}

int Fonts::LoadFont(std::string filepath)
{
	if (device == nullptr) return -1;

	console.Print("Loading font: %s", filepath);

	std::fstream file = std::fstream(filepath);

	// Convert our filepath string to a wide string, because Windows likes wide characters
	std::wstring wideString(filepath.length(), ' ');
	std::copy(filepath.begin(), filepath.end(), wideString.begin());

	if (file.fail())
	{
		console.Print(MsgType::FAILED, "Font loading failed: %s", filepath);
		file.close();
		return -1;
	}

	file.close();

	console.Print("Font was loaded");
	fonts.push_back(std::make_shared<SpriteFont>(device.Get(), wideString.c_str()));

	return fonts.size() - 1;
}

SpriteFont* Fonts::Get(int fontIndex)
{
	if (fontIndex < 0 || fontIndex >= fonts.size()) return nullptr;
	return fonts.at(fontIndex).get();
}
