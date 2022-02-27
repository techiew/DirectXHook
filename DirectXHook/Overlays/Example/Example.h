#pragma once

#include "IRenderCallback.h"
#include "OverlayFramework.h"

class Example : public IRenderCallback
{
public:
	void Setup();
	void Render();

private:
	OF::Box* box1;
	OF::Box* box2;
	OF::Box* box3;
	int texture1 = 0;
	int texture2 = 0;
	int texture3 = 0;
};