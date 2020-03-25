#pragma once
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace CoreAttributes
{

	// Attributes that affect the graphics of an element
	struct Look
	{
		bool visible;
		int texture;
		Color color;
		float rotation;
		bool showIfParentVisible; // Only show the element if the parent is also visible
		SpriteEffects spriteEffect; // Used to flip the sprite horizontally, vertically, or both.

		Look
		(
			bool visible = true,
			int texture = 0,
			Color color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			float rotation = 0.0f,
			bool showIfParentVisible = true,
			SpriteEffects spriteEffect = SpriteEffects_None
		)
		{
			this->visible = visible;
			this->texture = texture;
			this->color = color;
			this->rotation = rotation;
			this->showIfParentVisible = showIfParentVisible;
			this->spriteEffect = spriteEffect;
		}
	};

	// Attributes that affect responses to actions by the user
	struct Behavior
	{
		bool moveable;
		bool clickable;
		bool resizable;
		bool clickthrough;
		bool followParent;
		bool onTopOfParent;
		bool stayInsideParent;

		Behavior
		(
			bool moveable = true,
			bool clickable = true,
			bool resizable = true,
			bool clickthrough = false,
			bool followParent = false,
			bool onTopOfParent = true,
			bool stayInsideParent = false
		)
		{
			this->moveable = moveable;
			this->clickable = clickable;
			this->resizable = resizable;
			this->clickthrough = clickthrough;
			this->followParent = followParent;
			this->onTopOfParent = onTopOfParent;
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

	struct Data
	{
		std::string text;
		int fontIndex;

		Data
		(
			std::string text = "",
			int fontIndex = -1
		)
		{
			this->text = text;
			this->fontIndex = fontIndex;
		}
	};

	static const Look defaultLook;
	static const Behavior defaultBehavior;
	static const Scaling defaultScaling;
	static const Data defaultData;

	// Common attributes for all UI elements
	struct Attributes
	{
		Look look;
		Behavior behavior;
		Scaling scaling;
		Data data;

		Attributes(Look look = defaultLook, Behavior behavior = defaultBehavior, Scaling scaling = defaultScaling, Data data = defaultData)
		{
			this->look = look;
			this->behavior = behavior;
			this->scaling = scaling;
			this->data = data;
		}
	};

}