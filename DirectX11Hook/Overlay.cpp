#include "Overlay.h"

Overlay::Overlay(ID3D11Device* device, SpriteBatch* spriteBatch, DebugConsole* console, HWND window)
{
	this->spriteBatch = std::shared_ptr<SpriteBatch>(spriteBatch);
	this->console = console;
	this->window = window;
	textures = Textures(device, console);
	fonts = Fonts(device, console);

	RECT hwndRect;
	GetClientRect(this->window, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;

	uiPos.resize(ID::_NUMELEMENTS, XMFLOAT3(0.0f, 0.0f, 0.0f));
	uiSize.resize(ID::_NUMELEMENTS, XMFLOAT2(100.0f, 100.0f));
	uiAttrib.resize(ID::_NUMELEMENTS, Attributes());
	uiParent.resize(ID::_NUMELEMENTS, ID::_NONE);
	uiChildren.resize(ID::_NUMELEMENTS, std::vector<bool>(ID::_NUMELEMENTS, false));
	numChildren.resize(ID::_NUMELEMENTS, 0);

	// Blank texture, will be on texture index 0
	textures.LoadTexture(".\\hook_textures\\blank.jpg");

	Load();
	ReadConfig();
	SortByDepth();
	console->PrintDebugMsg("Loading complete, now rendering...", nullptr, MsgType::STARTPROCESS);
}

void Overlay::Update()
{
	CheckMouseState();

	spriteBatch->Begin(SpriteSortMode_FrontToBack);
	Draw();
	spriteBatch->End();
}

void Overlay::Signup(ID id)
{
	configSignups.push_back(id);
}

void Overlay::ReadConfig()
{
}

bool Overlay::DoBox(ID id)
{
	ID3D11ShaderResourceView* texture = textures.Get(GetTexture(id));
	if (texture == nullptr) return false;

	XMFLOAT3 pos = uiPos[id];
	XMFLOAT2 size = uiSize[id];

	bool clicked = false;

	// Check if element is currently being hovered over with the mouse 
	// or if the mouse is currently locked on (dragging) this element.
	if (id == hoveringOverID || (mouseLocked && activeID == id))
	{
		clicked = CheckElementClicked(id);
	}

	RECT rect;
	XMFLOAT2 parentPos = XMFLOAT2(0.0f, 0.0f);

	if (uiParent[id] != ID::_NONE)
	{
		parentPos = XMFLOAT2(uiPos[uiParent[id]].x, uiPos[uiParent[id]].y);
	}

	if (GetBehavior(id)->followParent)
	{
		rect.top = pos.y + parentPos.y;
		rect.left = pos.x + parentPos.x;
		rect.bottom = pos.y + size.y + parentPos.y;
		rect.right = pos.x + size.x + parentPos.x;
	}
	else
	{
		rect.top = pos.y;
		rect.left = pos.x;
		rect.bottom = pos.y + size.y;
		rect.right = pos.x + size.x;
	}

	spriteBatch->Draw(texture, rect, NULL, uiAttrib[id].look.color, uiAttrib[id].look.rotation, XMFLOAT2(0.0f, 0.0f), uiAttrib[id].look.spriteEffect, pos.z);

	return clicked;
}

bool Overlay::DoText(ID id, int fontIndex)
{
	return false;
}

//bool Overlay::DoBox(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, int texIndex, float rotation, DirectX::SimpleMath::Color color, bool moveable, bool clickable, DirectX::SpriteEffects spriteEffects)
//{
//	return false;
//}
//
//bool Overlay::DoText(const char* text, DirectX::XMFLOAT3 pos, float size, int fontIndex, float rotation, DirectX::SimpleMath::Color color)
//{
//	SpriteFont* font = fonts.Get(fontIndex);
//	if (font == nullptr) return false;
//	font->DrawString(spriteBatch.get(), text, XMFLOAT2(pos.x, pos.y), color, 0.0f, XMFLOAT2(0.0f, 0.0f), size, DirectX::SpriteEffects_None, pos.z);
//	return false;
//}


void Overlay::CheckMouseState()
{
	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(window, &pos);

	deltaMousePos = XMFLOAT2(pos.x - mousePos.x, pos.y - mousePos.y);
	mousePos = XMFLOAT2(pos.x, pos.y);

	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{

		if (!mouseLeftPressed)
		{
			mouseLeftPressedTime = GetTimeMs();
		}

		mouseLeftPressed = true;
	}
	else
	{
		mouseLocked = false;
		mouseLeftPressed = false;
	}

	int elementID = ID::_NONE;
	float highestZ = 0.0f;

	for (int i = 0; i < uiPos.size(); i++)
	{
		if (uiAttrib[i].look.visible == false) continue;

		XMFLOAT3 pos = uiPos[i];
		XMFLOAT2 size = uiSize[i];

		if (uiParent[i] != ID::_NONE && GetBehavior((ID)i)->followParent)
		{
			pos = XMFLOAT3(pos.x + uiPos[uiParent[i]].x, pos.y + uiPos[uiParent[i]].y, pos.z);
		}

		if (IsMouseInside(pos, size))
		{

			if (pos.z >= highestZ)
			{
				highestZ = pos.z;
				elementID = i;
			}

		}

	}

	hoveringOverID = elementID;
}

bool Overlay::IsMouseInside(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 area)
{

	if (mousePos.x < (origin.x + area.x) && mousePos.x >(origin.x))
	{

		if (mousePos.y < (origin.y + area.y) && mousePos.y >(origin.y))
		{
			return true;
		}

	}

	return false;
}

bool Overlay::CheckElementClicked(int element)
{
	if (!uiAttrib[element].behavior.clickable) return false;

	if (mouseLeftPressed)
	{

		if (mouseLocked)
		{

			if (activeID == element)
			{

				if (stopMoreClicksOnID != element)
				{
					stopMoreClicksOnID = element;
					OnClick(element);
				}

				DragElementWithMouse(element);
				return true;
			}

			return false;
		}

		mouseLocked = true;
		activeID = element;
		stopMoreClicksOnID = ID::_NONE;
	}
	else if (activeID == element)
	{

		if (GetTimeMs().count() - mouseLeftPressedTime.count() < 400)
		{

			if (stopMoreClicksOnID != element)
			{
				stopMoreClicksOnID = element;
				OnClick(element);
				return true;
			}

		}

	}

}

void Overlay::OnClick(int element)
{
	elementClickPos = XMFLOAT2(mousePos.x - uiPos[element].x, mousePos.y - uiPos[element].y);

	if (uiParent[element] != ID::_NONE)
	{

		if (GetBehavior((ID)element)->onTopOfParent)
		{
			OnClick(uiParent[element]);

			if (GetBehavior((ID)element)->followParent)
			{
				elementClickPos = XMFLOAT2(mousePos.x - (uiPos[element].x + uiPos[uiParent[element]].x), mousePos.y - (uiPos[element].y + uiPos[uiParent[element]].y));
			}
			else
			{
				elementClickPos = XMFLOAT2(mousePos.x - uiPos[element].x, mousePos.y - uiPos[element].y);
			}

			return;
		}

		if (GetBehavior((ID)element)->followParent)
		{
			elementClickPos = XMFLOAT2(mousePos.x - (uiPos[element].x + uiPos[uiParent[element]].x), mousePos.y - (uiPos[element].y + uiPos[uiParent[element]].y));
		}

	}

	if (uiPos[element].z == ((float)(uiPos.size() - 1)) / numLayers) return;

	float oldZ = uiPos[element].z;
	uiPos[element].z = ((float)(uiPos.size() - 1)) / numLayers;

	for (int i = 0; i < uiPos.size(); i++)
	{
		if (i == element) continue;

		if (uiParent[i] == element)
		{
			uiPos[element].z = uiPos[uiParent[element]].z + (1.0f / (numLayers * 10));
			continue;
		}

		if (uiPos[i].z > oldZ) uiPos[i].z -= (1.0f / numLayers);
	}

}

void Overlay::DragElementWithMouse(int element)
{

	if (!uiAttrib[element].behavior.resizable && uiAttrib[element].behavior.moveable)
	{
		uiPos[element] = XMFLOAT3(uiPos[element].x + deltaMousePos.x, uiPos[element].y + deltaMousePos.y, uiPos[element].z);
		return;
	}

	int xEdge = 0;
	int yEdge = 0;

	if (elementClickPos.x < edgeMargin)
	{
		xEdge = -1;
	}
	else if (uiSize[element].x - elementClickPos.x < edgeMargin)
	{
		xEdge = 1;
	}

	if (elementClickPos.y < edgeMargin)
	{
		yEdge = -1;
	}
	else if (uiSize[element].y - elementClickPos.y < edgeMargin)
	{
		yEdge = 1;
	}

	if (xEdge == 0 && yEdge == 0 && uiAttrib[element].behavior.moveable)
	{
		uiPos[element] = XMFLOAT3(uiPos[element].x + deltaMousePos.x, uiPos[element].y + deltaMousePos.y, uiPos[element].z);
	}
	else
	{
		float diffX = uiSize[element].x - ((float)edgeMargin * 3);
		float diffY = uiSize[element].y - ((float)edgeMargin * 3);

		if (xEdge == 1)
		{

			if ((uiSize[element].x + deltaMousePos.x) >= ((float)edgeMargin * 3))
			{
				uiSize[element].x += deltaMousePos.x;
				elementClickPos.x += deltaMousePos.x;
			}
			else
			{
				uiSize[element].x = ((float)edgeMargin * 3);
				elementClickPos.x += diffX;
			}

		}
		else if (xEdge == -1)
		{

			if ((uiSize[element].x - deltaMousePos.x) >= ((float)edgeMargin * 3))
			{
				uiPos[element] = XMFLOAT3(uiPos[element].x + deltaMousePos.x, uiPos[element].y, uiPos[element].z);
				uiSize[element].x -= deltaMousePos.x;
			}
			else
			{
				uiSize[element].x = ((float)edgeMargin * 3);
				elementClickPos.x -= diffX;
			}

		}

		if (yEdge == 1)
		{

			if ((uiSize[element].y + deltaMousePos.y) >= ((float)edgeMargin * 3))
			{
				uiSize[element].y += deltaMousePos.y;
				elementClickPos.y += deltaMousePos.y;
			}
			else
			{
				uiSize[element].y = ((float)edgeMargin * 3);
				elementClickPos.y -= diffY;
			}

		}
		else if (yEdge == -1) 
		{

			if ((uiSize[element].y - deltaMousePos.y) >= ((float)edgeMargin * 3))
			{
				uiPos[element] = XMFLOAT3(uiPos[element].x, uiPos[element].y + deltaMousePos.y, uiPos[element].z);
				uiSize[element].y -= deltaMousePos.y;
			}
			else
			{
				uiSize[element].y = ((float)edgeMargin * 3);
				elementClickPos.y -= diffY;
			}

		}

	}

	if (GetBehavior((ID)element)->stayInsideParent)
	{
		XMFLOAT3 thisPos = uiPos[element];
		XMFLOAT3 parentPos = uiPos[uiParent[element]];
		XMFLOAT2 thisSize = uiSize[element];
		XMFLOAT2 parentSize = uiSize[uiParent[element]];

		if ((thisPos.x + parentPos.x) < parentPos.x)
		{
			uiPos[element].x = 0;

			if (xEdge == -1 && (uiSize[element].x - deltaMousePos.x) >= ((float)edgeMargin * 3))
			{
				uiSize[element].x += deltaMousePos.x;
			}

		}
		else if ((thisPos.x + thisSize.x) > parentSize.x)
		{

			if (xEdge == 1)
			{
				uiSize[element].x = parentSize.x - uiPos[element].x;
			}
			else
			{
				uiPos[element].x = parentSize.x - uiSize[element].x;
			}

		}

		if ((thisPos.y + parentPos.y) < parentPos.y)
		{
			uiPos[element].y = 0;

			if (yEdge == -1 && (uiSize[element].y - deltaMousePos.y) >= ((float)edgeMargin * 3))
			{
				uiSize[element].y += deltaMousePos.y;
			}

		}
		else if ((thisPos.y + thisSize.y) > parentSize.y)
		{

			if (yEdge == 1)
			{
				uiSize[element].y = parentSize.y - uiPos[element].y;
			}
			else
			{
				uiPos[element].y = parentSize.y - uiSize[element].y;
			}

		}

	}

}

void Overlay::SortByDepth()
{

	struct IndexWithDepth
	{
		int i;
		float z;

		IndexWithDepth(int _i, float _z) : i(_i), z(_z) {};

		bool operator < (const IndexWithDepth& comp) const
		{
			return (z < comp.z);
		};
	};

	std::vector<IndexWithDepth> zBuffer = std::vector<IndexWithDepth>();

	for (int i = 0; i < uiPos.size(); i++)
	{
		IndexWithDepth indexWithDepth = { i, uiPos[i].z };
		zBuffer.push_back(indexWithDepth);
	}

	std::sort(zBuffer.begin(), zBuffer.end(), std::less<IndexWithDepth>());

	for (int i = 0; i < zBuffer.size(); i++)
	{
		uiPos[zBuffer[i].i].z = (float)i / numLayers;
	}

	for (int i = 0; i < uiChildren.size(); i++)
	{

		for (int j = 0; j < uiChildren[i].size(); j++)
		{
			if (uiChildren[i][j] == false) continue;

			if (GetBehavior((ID)j)->onTopOfParent)
			{
				uiPos[j].z = uiPos[(ID)i].z + (1.0f / (numLayers * 10));
			}

		}

	}

}

XMFLOAT2 Overlay::ToPixels(float ndcX, float ndcY)
{
	return XMFLOAT2(MapNumberInRange(ndcX, -1.0f, 1.0f, 0, windowWidth), MapNumberInRange(ndcY, -1.0f, 1.0f, 0, windowHeight));
}

DirectX::XMFLOAT2 Overlay::ToNDC(float pixelsX, float pixelsY)
{
	return XMFLOAT2(MapNumberInRange(pixelsX, 0, windowWidth, -1.0f, 1.0f), MapNumberInRange(pixelsY, 0, windowHeight, -1.0f, 1.0f));
}

DirectX::SimpleMath::Color Overlay::RGBAColor(float r, float g, float b, float a)
{
	float _r = MapNumberInRange(r, 0.0f, 255.0f, 0.0f, 1.0f);
	float _g = MapNumberInRange(g, 0.0f, 255.0f, 0.0f, 1.0f);
	float _b = MapNumberInRange(b, 0.0f, 255.0f, 0.0f, 1.0f);
	float _a = MapNumberInRange(a, 0.0f, 255.0f, 0.0f, 1.0f);
	return Color(_r, _g, _b, _a);
}

float Overlay::MapNumberInRange(float number, float inputStart, float inputEnd, float outputStart, float outputEnd)
{
	return outputStart + ((outputEnd - outputStart) / (inputEnd - inputStart)) * (number - inputStart);
}

/*
*
* Setters & getters ------------------------------------------------------------------------------
*
*/

void Overlay::SetAll(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, Look look, Behavior behavior, Scaling scaling)
{
	uiPos[id] = pos;
	uiSize[id] = size;
	SetLook(id, look);
	SetBehavior(id, behavior);
	SetScaling(id, scaling);
}

void Overlay::SetPosSize(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size)
{
	uiPos[id] = pos;
	uiSize[id] = size;
}

void Overlay::SetAttrib(ID id, Attributes attrib)
{
	uiAttrib[id] = attrib;
}

void Overlay::SetLook(ID id, Look look)
{
	GetAttrib(id)->look = look;
}

void Overlay::SetBehavior(ID id, Behavior behavior)
{
	GetAttrib(id)->behavior = behavior;
}

void Overlay::SetScaling(ID id, Scaling scaling)
{
	GetAttrib(id)->scaling = scaling;
}

void Overlay::SetPos(ID id, DirectX::XMFLOAT3 pos)
{
	uiPos[id] = pos;
}

void Overlay::SetSize(ID id, DirectX::XMFLOAT2 size)
{
	uiSize[id] = size;
}

void Overlay::SetColor(ID id, Color color)
{
	GetAttrib(id)->look.color = color;
}

void Overlay::SetTexture(ID id, int texIndex)
{
	GetAttrib(id)->look.texture = texIndex;
}

void Overlay::SetParent(ID id, ID parent)
{

	if (id == parent) return;

	if (uiParent[id] != ID::_NONE)
	{
		uiChildren[uiParent[id]][id] = false;
		numChildren[uiParent[id]] -= 1;
	}

	uiChildren[parent][id] = true;
	numChildren[parent] += 1;
	uiParent[id] = parent;

	if (GetBehavior(id)->onTopOfParent)
	{
		uiPos[id].z = uiPos[parent].z + (1.0f / (numLayers * 10));
	}

}

void Overlay::SetRotation(ID id, float rotation)
{
	GetAttrib(id)->look.rotation = rotation;
}

void Overlay::SetWindowHandle(HWND window)
{
	this->window = window;
	RECT hwndRect;
	GetClientRect(this->window, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;
}

DirectX::XMFLOAT3 Overlay::GetPos(ID id)
{
	return uiPos[id];
}

DirectX::XMFLOAT2 Overlay::GetSize(ID id)
{
	return uiSize[id];
}

Attributes* Overlay::GetAttrib(ID id)
{
	return &uiAttrib[id];
}

Look* Overlay::GetLook(ID id)
{
	return &uiAttrib[id].look;
}

Behavior* Overlay::GetBehavior(ID id)
{
	return &uiAttrib[id].behavior;
}

Scaling* Overlay::GetScaling(ID id)
{
	return &uiAttrib[id].scaling;
}

DirectX::SimpleMath::Color Overlay::GetColor(ID id)
{
	return uiAttrib[id].look.color;
}

int Overlay::GetTexture(ID id)
{
	return uiAttrib[id].look.texture;
}

ID Overlay::GetParent(ID id)
{
	return uiParent[id];
}

DirectX::XMFLOAT2 Overlay::GetMousePos()
{
	return mousePos;
}

DirectX::XMFLOAT2 Overlay::GetDeltaMousePos()
{
	return deltaMousePos;
}

std::chrono::milliseconds Overlay::GetMouseLeftPressTime()
{
	return mouseLeftPressedTime;
}

std::chrono::milliseconds Overlay::GetMouseRightPressTime()
{
	return mouseRightPressedTime;
}

bool Overlay::IsMouseLeftPressed()
{
	return mouseLeftPressed;
}

bool Overlay::IsMouseRightPressed()
{
	return mouseRightPressed;
}

bool Overlay::IsMouseLockedOnElement()
{
	return mouseLocked;
}

int Overlay::GetActiveID()
{
	return activeID;
}

int Overlay::GetHoveringOverID()
{
	return hoveringOverID;
}

int Overlay::GetStopClicksOnID()
{
	return stopMoreClicksOnID;
}

int Overlay::GetWindowWidth()
{
	return windowWidth;
}

int Overlay::GetWindowHeight()
{
	return windowHeight;
}

std::chrono::milliseconds Overlay::GetTimeMs()
{
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch());
}