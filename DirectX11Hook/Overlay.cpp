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

	uiPos.resize((ID::_NUMELEMENTS), XMFLOAT3(0.0f, 0.0f, 0.0f));
	uiSize.resize((ID::_NUMELEMENTS), XMFLOAT2(100.0f, 100.0f));
	uiAttrib.resize((ID::_NUMELEMENTS), nullptr);

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

void Overlay::ReadConfig()
{
}

bool Overlay::DoBox(ID id, int texIndex, ID parent)
{
	ID3D11ShaderResourceView* texture = textures.Get(texIndex);
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

	if (parent != ID::_NONE)
	{
		parentPos = XMFLOAT2(uiPos[(int)parent].x, uiPos[(int)parent].y);
	}

	rect.top = pos.y;
	rect.left = pos.x;
	rect.bottom = pos.y + size.y;
	rect.right = pos.x + size.x;

	spriteBatch->Draw(texture, rect, NULL, uiAttrib[id]->color, uiAttrib[id]->rotation, parentPos, DirectX::SpriteEffects_None, pos.z);

	return clicked;
}

bool Overlay::DoText(ID id, int fontIndex, ID parent)
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

	deltaMousePos = XMFLOAT2(mousePos.x - pos.x, mousePos.y - pos.y);
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

		if (IsMouseInside(uiPos[i], uiSize[i]))
		{

			if (uiPos[i].z >= highestZ)
			{
				highestZ = uiPos[i].z;
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
	if (!uiAttrib[element]->clickable) return false;

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

				if (uiAttrib[element]->moveable) DragElementWithMouse(element);
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
	if (uiPos[element].z == ((float)(uiPos.size() - 1)) / numLayers) return;

	float oldZ = uiPos[element].z;
	uiPos[element].z = ((float)(uiPos.size() - 1)) / numLayers;

	for (int i = 0; i < uiPos.size(); i++)
	{
		if (i == element) continue;
		if (uiPos[i].z > oldZ) uiPos[i].z -= (1.0f / numLayers);
	}

}

void Overlay::DragElementWithMouse(int element)
{
	uiPos[element] = XMFLOAT3(uiPos[element].x - deltaMousePos.x, uiPos[element].y - deltaMousePos.y, uiPos[element].z);
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
	return Color(r, g, b, a);
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

void Overlay::Signup(ID id)
{
	configSignups.push_back(id);
}

void Overlay::SetDef(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size)
{
	uiPos[id] = pos;
	uiSize[id] = size;
	uiAttrib[id] = std::make_shared<Attributes>(Attributes());
}

void Overlay::SetDef(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, Attributes attrib)
{
	uiPos[id] = pos;
	uiSize[id] = size;
	uiAttrib[id] = std::make_shared<Attributes>(Attributes(attrib));
}

void Overlay::SetAttrib(ID id, Attributes attrib)
{
	uiAttrib[id] = std::make_shared<Attributes>(Attributes(attrib));
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
	if (GetAttrib(id) == nullptr) return;
	GetAttrib(id)->color = color;
}

void Overlay::SetRotation(ID id, float rotation)
{
	if (GetAttrib(id) == nullptr) return;
	GetAttrib(id)->rotation = rotation;
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
	return uiAttrib[id].get();
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