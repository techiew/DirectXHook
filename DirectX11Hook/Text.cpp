#include "Text.h"

using namespace DirectX;

// Used here to map pixels to normalized device coords and vice versa
float Text::MapFloatValueInRange(float value, float valueMin, float valueMax, float rangeMin, float rangeMax)
{
	return (value - valueMin) * (rangeMax - rangeMin) / (valueMax - valueMin) + rangeMin;
}

// Measures text size as pixels, and then returns the size as normalized device coords
XMFLOAT2 Text::MeasureTextSize(const char* text, DirectX::SpriteFont* font, int windowWidth, int windowHeight)
{
	XMFLOAT2 textSize;
	XMVECTOR textVector = font->MeasureString(text);
	XMStoreFloat2(&textSize, textVector);

	return XMFLOAT2
	(
		MapFloatValueInRange(textSize.x, 0, windowWidth, 0.0f, 2.0f),
		MapFloatValueInRange(textSize.y, 0, windowHeight, 0.0f, 2.0f)
	);
}

Text::Text(const char* text, float posX, float posY, int fontIndex)
{
	this->text = text;
	this->pos = XMFLOAT2(posX, posY);
	this->fontIndex = fontIndex;
}

Text::Text(const char* text, float posX, float posY, int fontIndex, DirectX::SpriteFont* font, int windowWidth, int windowHeight)
{
	this->text = text;
	pos = XMFLOAT2(posX, posY);
	this->fontIndex = fontIndex;
	if (font == nullptr) return;
	textSize = MeasureTextSize(text, font, windowWidth, windowHeight);
}

void Text::SetPos(float posX, float posY)
{
	pos = XMFLOAT2(posX, posY);
}

const char* Text::GetText()
{
	return text;
}

DirectX::XMFLOAT2 Text::GetTextSize()
{
	return textSize;
}

DirectX::XMFLOAT2 Text::GetPos()
{
	return pos;
}

int Text::GetFontIndex()
{
	return fontIndex;
}

// Convert from normalized device coords to pixels
DirectX::XMFLOAT2 Text::GetPosPixels(int windowWidth, int windowHeight)
{
	return DirectX::XMFLOAT2
	(
		MapFloatValueInRange(pos.x, -1.0f, 1.0f, 0, windowWidth),
		MapFloatValueInRange(pos.y, -1.0f, 1.0f, 0, windowHeight)
	);
}

float Text::GetTextMidpointX()
{
	return textSize.x / 2;
}

float Text::GetTextMidpointY()
{
	return textSize.y / 2;
}