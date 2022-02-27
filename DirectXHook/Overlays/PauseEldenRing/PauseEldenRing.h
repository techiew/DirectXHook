#pragma once

#include "IRenderCallback.h"
#include "OverlayFramework.h"

class PauseEldenRing : public IRenderCallback
{
public:
	void Setup();
	void Render();

private:
	OF::Box* m_pauseWindow;
	OF::Box* m_topBar;
	OF::Box* m_bottomBar;
	int m_font = 0;
	int m_barTexture = 0;
	int m_rotatedBarTexture = 0;
	bool m_gamePaused = false;
};