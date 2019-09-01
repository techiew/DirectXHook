#pragma once
#include <string>
#include <DirectXMath.h>
#include <SpriteFont.h>

class Text
{
private: 
	const char* text;
	DirectX::XMFLOAT2 textSize = DirectX::XMFLOAT2(1.0f, 1.0f);
	DirectX::XMFLOAT2 pos = DirectX::XMFLOAT2(0.0f, 0.0f); // Normalized device coordinates
	int fontIndex;

	static float MapFloatValueInRange(float value, float valueMin, float valueMax, float rangeMin, float rangeMax);
public:
	Text(const char* text, float posX, float posY, int fontIndex);
	Text(const char* text, float posX, float posY, int fontIndex, DirectX::SpriteFont* font, int windowWidth, int windowHeight);
	void SetPos(float posX, float posY);
	const char* GetText();
	DirectX::XMFLOAT2 GetTextSize();
	DirectX::XMFLOAT2 GetPos();
	int GetFontIndex();
	DirectX::XMFLOAT2 GetPosPixels(int windowWidth, int windowHeight);
	float GetTextMidpointX();
	float GetTextMidpointY();

	static DirectX::XMFLOAT2 MeasureTextSize(const char* text, DirectX::SpriteFont* font, int windowWidth, int windowHeight);
};