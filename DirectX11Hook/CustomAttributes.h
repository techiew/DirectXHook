#pragma once
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace CustomAttributes
{

	// Attributes that affect the graphics of an element
	struct Look
	{
		bool visible;
		int texture;
		Color color;
		float rotation;
		SpriteEffects spriteEffect; // Used to flip the sprite horizontally, vertically, or both.

		Look
		(
			bool visible = true,
			int texture = 0,
			Color color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			float rotation = 0.0f,
			SpriteEffects spriteEffect = SpriteEffects_None
		)
		{
			this->visible = visible;
			this->texture = texture;
			this->color = color;
			this->rotation = rotation;
			this->spriteEffect = spriteEffect;
		}
	};

	// Attributes that affect responses to actions by the user
	struct Behavior
	{
		bool moveable;
		bool clickable;
		bool resizable;
		bool onTopOfParent;
		bool followParent;
		bool stayInsideParent;

		Behavior
		(
			bool moveable = true,
			bool clickable = true,
			bool resizable = true,
			bool onTopOfParent = true,
			bool followParent = true,
			bool stayInsideParent = false
		)
		{
			this->moveable = moveable;
			this->clickable = clickable;
			this->resizable = resizable;
			this->onTopOfParent = onTopOfParent;
			this->followParent = followParent;
			this->stayInsideParent = stayInsideParent;
		}
	};

	// Attributes that affect how an element scales inside its parent
	struct Scaling
	{
		bool stretchX;
		bool stretchY;
		bool shrinkX;
		bool shrinkY;
		XMFLOAT2 prefSize; // The element will stay this size if possible

		Scaling
		(
			bool stretchX = false,
			bool stretchY = false,
			bool shrinkX = false,
			bool shrinkY = false,
			XMFLOAT2 prefSize = XMFLOAT2(0.0f, 0.0f)
		)
		{
			this->stretchX = stretchX;
			this->stretchY = stretchY;
			this->shrinkX = shrinkX;
			this->shrinkY = shrinkY;
			this->prefSize = prefSize;
		}
	};

	static Look defaultLook;
	static Behavior defaultBehavior;
	static Scaling defaultScaling;

	// Common attributes for all UI elements
	struct Attributes
	{
		Look look;
		Behavior behavior;
		Scaling scaling;

		Attributes(Look look = defaultLook, Behavior behavior = defaultBehavior, Scaling scaling = defaultScaling)
		{
			this->look = look;
			this->behavior = behavior;
			this->scaling = scaling;
		}
	};

}