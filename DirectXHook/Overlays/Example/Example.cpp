#include "Example.h"

using namespace OF;

void Example::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);
	box1 = CreateBox(100, 100, 100, 100);
	box2 = CreateBox(150, 150, 100, 100);
	box3 = CreateBox(200, 200, 100, 100);
	texture1 = LoadTexture("hook_textures\\texture1.png");
	texture2 = LoadTexture("hook_textures\\texture2.png");
	texture3 = LoadTexture("hook_textures\\texture3.png");                                   
}

void Example::Render()
{
	DrawBox(box1, texture1);
	DrawBox(box2, texture2);
	DrawBox(box3, texture3);
}