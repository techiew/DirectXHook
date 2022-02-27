#include "Example.h"

using namespace OF;

void Example::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);
	box = CreateBox(100, 100, 100, 100);   
	font = LoadFont("hook_fonts\\OpenSans-22.spritefont");
	SetFont(font);
}

void Example::Render()
{
	DrawText(box, "This is some red text", 0, 0, 1.0f, 255, 0, 0);
	DrawText(box, "This is some green and smaller text", 0, 40, 0.7f, 0, 255, 0);
	DrawText(box, "This is some blue text at an angle", 0, 80, 0.7f, 50, 50, 255, 255, 0.7f);
}