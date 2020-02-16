#include "Overlay.h"

void Overlay::Load()
{
	// Example textures
	tex.doggo1 = textures.LoadTexture(".\\hook_textures\\texture.jpg");
	tex.doggo2 = textures.LoadTexture(".\\hook_textures\\texture2.jpg");

	// Default font
	font.arial = fonts.LoadFont(".\\hook_fonts\\arial_22.spritefont");

	SetPosSize(DOGGO1, Vec3(500, 500, 0.8f), Vec2(300, 300));
	SetPosSize(DOGGO2, Vec3(0, 0, 0.9f), Vec2(200, 200));

	//SetTexture(DOGGO1, tex.doggo1);
	//SetTexture(DOGGO2, tex.doggo2);

	SetColor(DOGGO2, RGBAColor(255, 0, 0, 255));

	SetParent(DOGGO2, DOGGO1);

	GetBehavior(DOGGO2)->stayInsideParent = true;

	Signup(DOGGO1);
	Signup(DOGGO2);
}

void Overlay::Draw()
{

	if (DoBox(DOGGO1))
	{

	}

	if (DoBox(DOGGO2))
	{

	}

}