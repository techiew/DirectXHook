#pragma once

#include "DebugConsole.h"
#include "Textures.h"
#include "Fonts.h"
#include "DebugConsole.h"
#include <SpriteBatch.h>

class Renderer;
class Textures;

class Overlay
{
private:
	Textures* textures;
	Fonts* fonts;
	DebugConsole* console;
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;

	void ReadValues();
	void ReadConfig();

	//bool DoRect(Vector2 pos, vector2 size, Color color, bool* hover);
	bool DoBox(DirectX::XMINT2 pos, DirectX::XMINT2 size, ID3D11ShaderResourceView* texture, bool* hover);
	//bool DoText(Vector2 pos, int size, Color color, bool* hover);
	//void DoBezierCurve(bool* hover);
	//void CheckMouseEvents(Vector2 pos, Vector2 size);

public:
	Overlay() {};
	Overlay(Renderer* renderer, DebugConsole* console);
	void Draw();
	void LoadEverything();
	void SetSpriteBatch(DirectX::SpriteBatch* spriteBatch);
};