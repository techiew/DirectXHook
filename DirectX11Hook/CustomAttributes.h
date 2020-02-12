#pragma once
using namespace DirectX;
using namespace DirectX::SimpleMath;

// Common attributes for all UI elements
class Attributes
{
public:
	bool visible = true;
	Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	bool moveable = true;
	bool clickable = true;
	bool resizable = true;
	float rotation = 0.0f;
	SpriteEffects spriteEffect = DirectX::SpriteEffects_None; // Used to flip the sprite horizontally, vertically, or both.

	Attributes
	(
		bool _visible = true,
		Color _color = Color(1.0f, 1.0f, 1.0f, 1.0f),
		bool _moveable = true,
		bool _clickable = true,
		bool _resizable = true,
		float _rotation = 0.0f,
		SpriteEffects _spriteEffect = DirectX::SpriteEffects_None
	)
	{
		visible = _visible;
		color = _color;
		moveable = _moveable;
		clickable = _clickable;
		resizable = _resizable;
		rotation = _rotation;
		spriteEffect = _spriteEffect;
	}

};