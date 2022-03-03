#pragma once

#include "IRenderCallback.h"
#include "OverlayFramework.h"

class PauseEldenRing : public IRenderCallback
{
public:
	void Setup();
	void Render();

private:
	std::fstream m_configFile;
	std::string m_configFileName = "pause_keybind.txt";
	unsigned char m_keybind = 'P';
	OF::Box* m_pauseWindow;
	OF::Box* m_topBar;
	OF::Box* m_bottomBar;
	int m_font = 0;
	int m_barTexture = 0;
	int m_rotatedBarTexture = 0;
	bool m_gamePaused = false;

	void ReadConfigFile(unsigned char* keybind);
};