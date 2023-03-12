#include "PauseTheGame.h"

using namespace OF;

void PauseTheGame::Setup()
{
	InitFramework(device, spriteBatch, window);
	ReadConfigFile(&keybind);
	pauseWindow = CreateBox(ofWindowWidth / 2 - 200, ofWindowHeight / 2 - 100, 400, 200);
	topBar = CreateBox(pauseWindow, 0, 0, pauseWindow->width, 7);
	bottomBar = CreateBox(pauseWindow, 0, pauseWindow->height, pauseWindow->width, 7);
	font = LoadFont("hook_fonts\\OpenSans-22.spritefont");
	barTexture = LoadTexture("hook_textures\\bar.png");
	rotatedBarTexture = LoadTexture("hook_textures\\bar_rotated.png");
	SetFont(font);
}

void PauseTheGame::Render()
{
	while (gamePaused)
	{
		if (MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLINPUT) == WAIT_OBJECT_0)
		{
			if (CheckHotkey(keybind))
			{
				gamePaused = false;
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

	if (CheckHotkey(keybind))
	{
		gamePaused = true;
		DrawBox(pauseWindow, 0, 0, 0, 240);
		DrawBox(topBar, barTexture);
		DrawBox(bottomBar, rotatedBarTexture);
		DrawText(pauseWindow, "Game paused.", 100, 80);
	}
}

void PauseTheGame::ReadConfigFile(unsigned int* keybind)
{
	configFile.open(configFileName, std::fstream::in);
	if (configFile.is_open())
	{
		std::string line = "";
		getline(configFile, line);
		if (line.length() < 3)
		{
			*keybind = 'P';
		}
		else
		{
			std::stringstream stringStream(line.substr(2, line.length()));
			logger.Log("Read keybind line: %s", line);
			stringStream >> std::hex >> *keybind;
			logger.Log("Keybind is: 0x%x", *keybind);
		}
		configFile.close();
	}
	else
	{
		logger.Log("Using default keybind");
		configFile.open(configFileName, std::fstream::out);
		if (configFile.is_open())
		{
			configFile << "0x50";
			configFile.close();
		}
	}
}
