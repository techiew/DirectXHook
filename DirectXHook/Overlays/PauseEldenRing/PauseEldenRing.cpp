#include "PauseEldenRing.h"

using namespace OF;

void PauseEldenRing::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);
	ReadConfigFile(&m_keybind);
	m_pauseWindow = CreateBox(ofWindowWidth / 2 - 200, ofWindowHeight / 2 - 100, 400, 200);
	m_topBar = CreateBox(m_pauseWindow, 0, 0, m_pauseWindow->width, 7);
	m_bottomBar = CreateBox(m_pauseWindow, 0, m_pauseWindow->height, m_pauseWindow->width, 7);
	m_font = LoadFont("hook_fonts\\OpenSans-22.spritefont");
	m_barTexture = LoadTexture("hook_textures\\bar.png");
	m_rotatedBarTexture = LoadTexture("hook_textures\\bar_rotated.png");
	SetFont(m_font);
}

void PauseEldenRing::Render()
{
	while (m_gamePaused)
	{
		if (MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLINPUT) == WAIT_OBJECT_0)
		{
			if (CheckHotkey(m_keybind))
			{
				m_gamePaused = false;
				return;
			}
			MSG msg;
			if (GetMessage(&msg, NULL, 0, 0) != -1)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	if (CheckHotkey(m_keybind))
	{
		m_gamePaused = true;
		DrawBox(m_pauseWindow, 0, 0, 0, 240);
		DrawBox(m_topBar, m_barTexture);
		DrawBox(m_bottomBar, m_rotatedBarTexture);
		DrawText(m_pauseWindow, "Game paused.", 100, 80);
	}
}

void PauseEldenRing::ReadConfigFile(unsigned int* keybind)
{
	m_configFile.open(m_configFileName, std::fstream::in);
	if (m_configFile.is_open())
	{
		std::string line = "";
		getline(m_configFile, line);
		if (line.length() < 3)
		{
			*keybind = 'P';
		}
		else
		{
			std::stringstream stringStream(line.substr(2, line.length()));
			m_logger.Log("Read keybind line: %s", line);
			stringStream >> std::hex >> *keybind;
			m_logger.Log("Keybind is: 0x%x", *keybind);
		}
		m_configFile.close();
	}
	else
	{
		m_logger.Log("Using default keybind");
		m_configFile.open(m_configFileName, std::fstream::out);
		if (m_configFile.is_open())
		{
			m_configFile << "0x50";
			m_configFile.close();
		}
	}
}
