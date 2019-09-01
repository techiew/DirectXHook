#include "Fonts.h"

using namespace DirectX;

Fonts::Fonts(DebugConsole * console)
{
	this->console = console;
}

// We need the DirectX device to load fonts
void Fonts::SetDevice(ID3D11Device* device)
{
	this->device = device;
}

int Fonts::LoadFont(std::string filepath)
{
	if (device == nullptr) return -1;

	console->PrintDebugMsg("Loading font: %s", (void*)filepath.c_str(), MsgType::PROGRESS);

	// Create sprite batch for text, also make an example font
	// We need to check if the font exists, if the file doesn't exist the program will crash
	std::fstream file = std::fstream(filepath);

	// Convert our filepath string to a wide string, because Windows likes wide characters
	std::wstring wideString(filepath.length(), ' ');
	std::copy(filepath.begin(), filepath.end(), wideString.begin());

	if (!file.fail())
	{
		file.close(); // Close the stream so we don't block the file
		fonts.push_back(std::make_shared<SpriteFont>(device.Get(), wideString.c_str()));
	}
	else
	{
		console->PrintDebugMsg("Font loading failed: %s, does the font file exist?", (void*)filepath.c_str(), MsgType::FAILED);
		fonts.push_back(nullptr);
		return -1;
	}

	return fonts.size() - 1;
}

SpriteFont* Fonts::GetFont(int fontIndex)
{
	return fonts.at(fontIndex).get();
}
