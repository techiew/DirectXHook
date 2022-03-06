#pragma once

#include "IRenderCallback.h"
#include "OverlayFramework.h"
#include "Logger.h"

class PauseEldenRing : public IRenderCallback
{
public:
	void Setup();
	void Render();

private:
	Logger m_logger{ "PauseEldenRing" };
	std::fstream m_configFile;
	std::string m_configFileName = "pause_keybind.txt";
	unsigned int m_keybind = 'P';
	OF::Box* m_pauseWindow = nullptr;
	OF::Box* m_topBar = nullptr;
	OF::Box* m_bottomBar = nullptr;
	int m_font = 0;
	int m_barTexture = 0;
	int m_rotatedBarTexture = 0;
	bool m_gamePaused = false;

	void ReadConfigFile(unsigned int* keybind);
};