#include "Overlay.h"

void Overlay::Load()
{
	// Blank texture
	tex.none = textures.LoadTexture(".\\hook_textures\\blank.jpg");

	// Example textures
	tex.doggo1 = textures.LoadTexture(".\\hook_textures\\texture.jpg");
	tex.doggo2 = textures.LoadTexture(".\\hook_textures\\texture2.jpg");

	// Default font
	font.arial = fonts.LoadFont(".\\hook_fonts\\arial_22.spritefont");

	SetDef(ID::DOGGO1, XMFLOAT3(500, 500, 0.0f), XMFLOAT2(300, 300));
	SetDef(ID::DOGGO2, XMFLOAT3(550, 550, 1.0f), XMFLOAT2(300, 300));
	SetDef(ID::DOGGO3, XMFLOAT3(600, 600, 0.0f), XMFLOAT2(300, 300));
	SetDef(ID::DOGGO4, XMFLOAT3(650, 650, 1.0f), XMFLOAT2(300, 300));
	Colors::AliceBlue;
	Color lol = RGBAColor(245, 245, 245, 255);
	console->PrintDebugMsg("Color R: %f", lol.x);
	console->PrintDebugMsg("Color G: %f", lol.y);
	console->PrintDebugMsg("Color B: %f", lol.z);
	console->PrintDebugMsg("Color A: %f", lol.w);
	GetAttrib(ID::DOGGO1)->color = lol;
	//SetAttrib(ID::DOGGO1, Attributes(true, RGBAColor(21, 25, 124, 255)));

	Signup(ID::DOGGO1);
	Signup(ID::DOGGO2);
}

void Overlay::Draw()
{

	if (DoBox(ID::DOGGO1, tex.none, ID::_NONE))
	{

	}

	if (DoBox(ID::DOGGO2, tex.doggo2, ID::_NONE))
	{

	}

	if (DoBox(ID::DOGGO3, tex.doggo1, ID::_NONE))
	{

	}

	if (DoBox(ID::DOGGO4, tex.doggo2, ID::_NONE))
	{

	}

}