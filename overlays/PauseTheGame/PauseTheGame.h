#pragma once

#include "IRenderCallback.h"
#include "OverlayFramework.h"
#include "Logger.h"

class PauseTheGame : public IRenderCallback
{
public:
	void Setup();
	void Render();

private:
	Logger logger{ "PauseTheGame" };
	std::fstream configFile;
	std::string configFileName = "pause_keybind.txt";
	unsigned int keybind = 'P';
	OF::Box* pauseWindow = nullptr;
	OF::Box* topBar = nullptr;
	OF::Box* bottomBar = nullptr;
	int font = 0;
	int barTexture = 0;
	int rotatedBarTexture = 0;
	bool gamePaused = false;

	void ReadConfigFile(unsigned int* keybind);
};