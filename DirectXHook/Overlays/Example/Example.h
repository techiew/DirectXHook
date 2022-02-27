#pragma once

#include "IRenderCallback.h"
#include "OverlayFramework.h"

class Example : public IRenderCallback
{
public:
	void Setup();
	void Render();

private:
	OF::Box* box;
	int font = 0;
};