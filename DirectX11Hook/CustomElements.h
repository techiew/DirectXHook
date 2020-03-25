#pragma once

namespace CustomElements
{

	// IDs for UI elements
	enum ID
	{
		MOUSE,
		WINDOW_MAIN,
		TEXT_DPS,

		// Put all new elements before these last two
		_NUMELEMENTS,
		_NONE
	};

	// Texture IDs
	struct Texture
	{

	};

	// Font IDs
	struct Font
	{
		// Default font, make sure to load it like this first to use it
		// font.arial = fonts.LoadFont(".\\hook_fonts\\arial_22.spritefont");
		int arial; 
	};

}