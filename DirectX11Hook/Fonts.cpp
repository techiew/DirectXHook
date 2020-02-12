#include "Fonts.h"

using namespace DirectX;

Fonts::Fonts(ID3D11Device* device, DebugConsole * console)
{
	this->device = device;
	this->console = console;
}

int Fonts::LoadFont(std::string filepath)
{
	if (device == nullptr) return -1;

	console->PrintDebugMsg("Loading font: %s", (void*)filepath.c_str());

	std::fstream file = std::fstream(filepath);

	// Convert our filepath string to a wide string, because Windows likes wide characters
	std::wstring wideString(filepath.length(), ' ');
	std::copy(filepath.begin(), filepath.end(), wideString.begin());

	if (file.fail())
	{
		console->PrintDebugMsg("Font loading failed: %s", (void*)filepath.c_str(), MsgType::FAILED);
		file.close();
		return -1;
	}

	file.close();

	console->PrintDebugMsg("Font was loaded");
	fonts.push_back(std::make_shared<SpriteFont>(device.Get(), wideString.c_str()));

	return fonts.size() - 1;
}

SpriteFont* Fonts::Get(int fontIndex)
{
	if (fontIndex < 0 || fontIndex >= fonts.size()) return nullptr;
	return fonts.at(fontIndex).get();
}
