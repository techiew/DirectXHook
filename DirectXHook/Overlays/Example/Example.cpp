#include "Example.h"

using namespace OF;

void Example::Setup()
{
	InitFramework(m_device, m_spriteBatch, m_window);
	box = CreateBox(100, 100, 100, 100);
}

void Example::Render()
{
	if (transformationPercentage > 1.0f)
	{
		transformationPercentage = 0.0f;
	}
	box->rotationDegrees = 360 * transformationPercentage;
	box->width = 100 * transformationPercentage + 100;
	box->height = 100 * transformationPercentage + 100;
	DrawBox(box, 255, 0, 0);
	transformationPercentage += 0.001;
}