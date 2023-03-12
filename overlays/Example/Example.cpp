#include "Example.h"

using namespace OF;

void Example::Setup()
{
	InitFramework(device, spriteBatch, window);
	box = CreateBox(100, 100, 100, 100);
}

void Example::Render()
{
	CheckMouseEvents();
	DrawBox(box, 255, 0, 0, 255);
}