#include "Example.h"

using namespace OF;

void Example::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);
	box1 = CreateBox(100, 100, 100, 100);
	box2 = CreateBox(150, 150, 100, 100);
	box3 = CreateBox(200, 200, 100, 100);
}

void Example::Render()
{
	DrawBox(box1, 255, 0, 0);
	DrawBox(box2, 0, 255, 0);
	DrawBox(box3, 0, 0, 255);
}