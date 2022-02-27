#include "PauseEldenRing.h"

using namespace OF;

void PauseEldenRing::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);
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
		if (CheckHotkey('P'))
		{
			m_gamePaused = false;
			return;
		}
		MSG msg;
		PeekMessage(&msg, m_window, 0, 0, PM_NOREMOVE);
	}

	if (CheckHotkey('P'))
	{
		m_gamePaused = true;
	}

	if (m_gamePaused)
	{
		DrawBox(m_pauseWindow, 0, 0, 0, 240);
		DrawBox(m_topBar, m_barTexture);
		DrawBox(m_bottomBar, m_rotatedBarTexture);
		DrawText(m_pauseWindow, "Game paused.", 100, 80);
	}
}