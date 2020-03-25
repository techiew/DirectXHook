#include "Overlay.h"

Overlay::Overlay(ID3D11Device* device, SpriteBatch* spriteBatch, DebugConsole* console, HWND window)
{
	this->spriteBatch = std::shared_ptr<SpriteBatch>(spriteBatch);
	this->console = console;
	this->window = window;
	textures = Textures(device, console);
	fonts = Fonts(device, console);
	scanner = SigScanner();
	mem = MemReader(console);

	RECT hwndRect;
	GetClientRect(this->window, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;

	uiPos.resize(ID::_NUMELEMENTS, XMFLOAT3(0.0f, 0.0f, 0.0f));
	uiSize.resize(ID::_NUMELEMENTS, XMFLOAT2(100.0f, 100.0f));
	uiAttrib.resize(ID::_NUMELEMENTS, CoreAttributes::Attributes());
	uiCustom.resize(ID::_NUMELEMENTS, CustomAttributes::Custom());
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

void Overlay::Render()
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
	ID3D11ShaderResourceView* texture = textures.Get(Texture(id));
	if (texture == nullptr) return false;

	bool clicked = CheckElementClicked(id);

	XMFLOAT3 pos = uiPos[id];
	XMFLOAT2 size = uiSize[id];
	XMFLOAT2 parentPos = XMFLOAT2(0.0f, 0.0f);

	if (Parent(id) != ID::_NONE && Behavior(id)->followParent)
	{
		parentPos = XMFLOAT2(uiPos[uiParent[id]].x, uiPos[uiParent[id]].y);
	}

	RECT rect;
	rect.top = pos.y + parentPos.y;
	rect.left = pos.x + parentPos.x;
	rect.bottom = pos.y + size.y + parentPos.y;
	rect.right = pos.x + size.x + parentPos.x;

	spriteBatch->Draw(texture, rect, NULL, uiAttrib[id].look.color, uiAttrib[id].look.rotation, XMFLOAT2(0.0f, 0.0f), uiAttrib[id].look.spriteEffect, pos.z);

	return clicked;
}

bool Overlay::DoText(ID id)
{
	SpriteFont* font = fonts.Get(Font(id));
	if (font == nullptr) return false;
	font->DrawString(spriteBatch.get(), Text(id).c_str(), XMFLOAT2(RealPos(id).x, RealPos(id).y), Color(id), 0.0f, XMFLOAT2(0.0f, 0.0f), uiSize[id], DirectX::SpriteEffects_None, uiPos[id].z);
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

	// Left mouse button
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
		mouseLeftLocked = false;
		mouseLeftPressed = false;
	}

	// Right mouse button
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
	{

		if (!mouseRightPressed)
		{
			mouseRightPressedTime = GetTimeMs();
		}

		mouseRightPressed = true;
	}
	else
	{
		mouseRightLocked = false;
		mouseRightPressed = false;
	}

	int elementID = ID::_NONE;
	float highestZ = 0.0f;

	// Get the ID of the topmost element that the cursor is hovering over
	for (int i = 0; i < uiPos.size(); i++)
	{
		if (uiAttrib[i].look.visible == false) continue;
		if (uiAttrib[i].behavior.clickthrough == true) continue;

		XMFLOAT3 pos = uiPos[i];
		XMFLOAT2 size = uiSize[i];

		if (uiParent[i] != ID::_NONE && Behavior((ID)i)->followParent)
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

// Handles checking for mouse presses/releases on UI elements.
bool Overlay::CheckElementClicked(int element)
{

	// If the element is not being hovered over, it can't be clicked.
	if (element != hoveringOverID && element != clickingID)
	{
		return false;
	}

	if (mouseLeftPressed)
	{

		if (!mouseLeftLocked)
		{
			mouseLeftLocked = true;
			clickingID = element;
			XMFLOAT3 elementPos = Pos((ID)element);
			XMFLOAT3 parentPos = Pos((ID)Parent((ID)element));
			relativeClickPos = XMFLOAT2(mousePos.x - (elementPos.x + parentPos.x), mousePos.y - (elementPos.y + parentPos.y));
		}

		if (clickingID == element)
		{
			DragOrResizeElement(element);
		}

	}
	else if (clickingID == element)
	{

		if (GetTimeMs().count() - mouseLeftPressedTime.count() < 600)
		{

			if (uiAttrib[element].behavior.clickable)
			{
				OnLeftClick(element);
				clickingID = ID::_NONE;
				return true;
			}

		}

		clickingID = ID::_NONE;
	}

	return false;
}

void Overlay::OnLeftClick(int element)
{

	if (uiParent[element] != ID::_NONE)
	{

		if (Behavior((ID)element)->onTopOfParent)
		{
			PlaceElementOnTop(uiParent[element]);
			activeID = element;
			return;
		}

	}

	PlaceElementOnTop((ID)element);
	activeID = element;
}

void Overlay::OnRightClick(int element)
{
}

// Handles moving and resizing elements using the mouse based on where the UI element was clicked.
void Overlay::DragOrResizeElement(int element)
{
	XMFLOAT3 tempPos = uiPos[element];
	XMFLOAT2 tempSize = uiSize[element];
	XMFLOAT2 tempRelativeClickPos = relativeClickPos;
	XMFLOAT3 parentPos = Pos(uiParent[element]);
	XMFLOAT2 parentSize = Size(uiParent[element]);
	int xResize = 0;
	int yResize = 0;

	// Check if clicked on the left edge
	if (tempRelativeClickPos.x < edgeMargin)
	{
		xResize = -1;
		tempPos.x += deltaMousePos.x;
		tempSize.x -= deltaMousePos.x;
	}
	// Check if clicked on the right edge
	else if (uiSize[element].x - tempRelativeClickPos.x < edgeMargin)
	{
		xResize = 1;
		tempSize.x += deltaMousePos.x;
		tempRelativeClickPos.x += deltaMousePos.x;
	}

	// Check if clicked on the top edge
	if (tempRelativeClickPos.y < edgeMargin)
	{
		yResize = -1;
		tempPos.y += deltaMousePos.y;
		tempSize.y -= deltaMousePos.y;
	}
	// Check if clicked on the bottom edge
	else if (uiSize[element].y - tempRelativeClickPos.y < edgeMargin)
	{
		yResize = 1;
		tempSize.y += deltaMousePos.y;
		tempRelativeClickPos.y += deltaMousePos.y;
	}

	// Now we need to check if the element is actually allowed to be resized by the temporary values
	// edgeMargin is the size of the clickable area for each edge
	if (tempSize.x < edgeMargin * 3)
	{

		if (xResize == -1)
		{
			tempPos.x -= (edgeMargin * 3) - tempSize.x;
			tempSize.x = edgeMargin * 3;
		}
		else
		{
			tempSize.x = edgeMargin * 3;
			tempRelativeClickPos.x = relativeClickPos.x;
		}

	}

	if (tempSize.y < edgeMargin * 3)
	{

		if (yResize == -1)
		{
			tempPos.y -= (edgeMargin * 3) - tempSize.y;
			tempSize.y = edgeMargin * 3;
		}
		else
		{
			tempSize.y = edgeMargin * 3;
			tempRelativeClickPos.y = relativeClickPos.y;
		}

	}

	// If the element was not clicked on the edges, we interpret it as a move instead.
	// Doesn't perform the move if constrained by a parent element.
	if (xResize == 0 && yResize == 0)
	{
		tempPos.x += deltaMousePos.x;
		tempPos.y += deltaMousePos.y;
	}

	// Check if the element should be constrained by a parent element
	if (uiParent[element] != ID::_NONE)
	{

		if (Behavior((ID)element)->stayInsideParent)
		{ 
			XMFLOAT2 result;
			result = IsFullyInsideArea(tempPos + parentPos, tempSize, parentPos, parentSize);

			if (result.x == -1)
			{

				if (xResize == 0)
				{
					tempPos.x = 0;
				}
				else
				{
					tempSize.x += tempPos.x;
					tempPos.x = 0;
				}

			}
			else if (result.x == 1)
			{

				if (xResize == 0)
				{
					tempPos.x -= (tempPos.x + tempSize.x - parentSize.x);
				}
				else
				{
					tempSize.x = parentSize.x - tempPos.x;
					tempPos.x -= (tempPos.x + tempSize.x - parentSize.x);
					tempRelativeClickPos.x = parentSize.x - tempPos.x;
				}

			}

			if (result.y == -1)
			{
				if (yResize == 0)
				{
					tempPos.y = 0;
				}
				else
				{
					tempSize.y += tempPos.y;
					tempPos.y = 0;
				}
			}
			else if (result.y == 1)
			{

				if (yResize == 0)
				{
					tempPos.y -= (tempPos.y + tempSize.y - parentSize.y);
				}
				else
				{
					tempSize.y = parentSize.y - tempPos.y;
					tempPos.y -= (tempPos.y + tempSize.y - parentSize.y);
					tempRelativeClickPos.y = parentSize.y - tempPos.y;
				}

			}

		}

	}

	// Set the actual variables equal to the temporary ones
	if (uiAttrib[element].behavior.moveable)
	{
		uiPos[element] = tempPos;
		relativeClickPos = tempRelativeClickPos;
	}

	if (uiAttrib[element].behavior.resizable)
	{
		uiSize[element] = tempSize;
		relativeClickPos = tempRelativeClickPos;
	}

}

bool Overlay::IsMouseInside(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 area)
{

	if (mousePos.x < (origin.x + area.x) && mousePos.x > (origin.x))
	{

		if (mousePos.y < (origin.y + area.y) && mousePos.y > (origin.y))
		{
			return true;
		}

	}

	return false;
}

XMFLOAT2 Overlay::IsFullyInsideArea(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT3 areaPos, XMFLOAT2 areaSize)
{
	float x = 0;
	float y = 0;

	if (pos.x < areaPos.x) x = -1;

	if (pos.y < areaPos.y) y = -1;
	
	if (pos.x + size.x > areaPos.x + areaSize.x) x = 1;

	if (pos.y + size.y > areaPos.y + areaSize.y) y = 1;

	return XMFLOAT2(x, y);
}

// Sorts all UI elements on different layers based on their Z position.
void Overlay::SortByDepth()
{

	// A struct so we can couple the Z position of the element with the index of the element in the uiPos vector.
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

	// Store these structs in a buffer which we will sort.
	std::vector<IndexWithDepth> zBuffer = std::vector<IndexWithDepth>();

	// Fill the structs and the buffer.
	for (int i = 0; i < uiPos.size(); i++)
	{
		IndexWithDepth indexWithDepth = { i, uiPos[i].z };
		zBuffer.push_back(indexWithDepth);
	}

	// Sort by using the structs < operator.
	std::sort(zBuffer.begin(), zBuffer.end(), std::less<IndexWithDepth>());

	// Fill all indices in uiPos with the sorted Z position.
	// Also maps the Z position to a number between 0.0f and 1.0f (The maximum layer depth is 1.0f when using spritebatch).
	for (int i = 0; i < zBuffer.size(); i++)
	{
		uiPos[zBuffer[i].i].z = (float)i / numLayers;
	}

	// Place child elements with the onTopOfParent attribute directly above the parent.
	for (int i = 0; i < uiChildren.size(); i++)
	{

		for (int j = 0; j < uiChildren[i].size(); j++)
		{
			if (uiChildren[i][j] == false) continue;

			if (Behavior((ID)j)->onTopOfParent)
			{
				uiPos[j].z = uiPos[(ID)i].z + (1.0f / (numLayers * 10));
			}

		}

	}

}

void Overlay::PlaceElementOnTop(ID element)
{

	if (uiPos[element].z == ((float)(uiPos.size() - 1)) / numLayers) return;

	float oldZ = uiPos[element].z;
	uiPos[element].z = ((float)(uiPos.size() - 1)) / numLayers;

	int numChildrenDisplaced = 0;

	// Loops through all other UI elements and decrements their Z pos (layer depth)
	// Treats child elements differently based on their attributes.
	for (int i = 0; i < uiPos.size(); i++)
	{
		if (i == element) continue;

		if (uiParent[i] == element)
		{

			if (Behavior((ID)i)->onTopOfParent)
			{
				uiPos[element].z = uiPos[uiParent[element]].z + (1.0f / (numLayers * (10 + numChildrenDisplaced)));
				numChildrenDisplaced++;
			}

			continue;
		}

		if (uiPos[i].z > oldZ) uiPos[i].z -= (1.0f / numLayers);
	}

}

XMFLOAT2 Overlay::NDC(float ndcX, float ndcY)
{
	return XMFLOAT2(MapNumberInRange(ndcX, -1.0f, 1.0f, 0, windowWidth), MapNumberInRange(ndcY, -1.0f, 1.0f, 0, windowHeight));
}

DirectX::XMFLOAT2 Overlay::PixelsToNDC(float pixelsX, float pixelsY)
{
	return XMFLOAT2(MapNumberInRange(pixelsX, 0, windowWidth, -1.0f, 1.0f), MapNumberInRange(pixelsY, 0, windowHeight, -1.0f, 1.0f));
}

DirectX::SimpleMath::Color Overlay::RGBA(float r, float g, float b, float a)
{
	float _r = MapNumberInRange(r, 0.0f, 255.0f, 0.0f, 1.0f);
	float _g = MapNumberInRange(g, 0.0f, 255.0f, 0.0f, 1.0f);
	float _b = MapNumberInRange(b, 0.0f, 255.0f, 0.0f, 1.0f);
	float _a = MapNumberInRange(a, 0.0f, 255.0f, 0.0f, 1.0f);
	return DirectX::SimpleMath::Color(_r, _g, _b, _a);
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

void Overlay::PosSize(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size)
{
	uiPos[id] = pos;
	uiSize[id] = size;
}

void Overlay::Look(ID id, CA::Look look)
{
	*Look(id) = look;
}

void Overlay::Behavior(ID id, CA::Behavior behavior)
{
	*Behavior(id) = behavior;
}

void Overlay::Scaling(ID id, CA::Scaling scaling)
{
	*Scaling(id) = scaling;
}

void Overlay::Pos(ID id, DirectX::XMFLOAT3 pos)
{
	uiPos[id] = pos;
}

void Overlay::Size(ID id, DirectX::XMFLOAT2 size)
{
	uiSize[id] = size;
}

void Overlay::Color(ID id, float r, float g, float b, float a)
{
	Look(id)->color = RGBA(r, g, b, a);
}

void Overlay::Texture(ID id, int texIndex)
{
	Look(id)->texture = texIndex;
}

void Overlay::Text(ID id, std::string text)
{
	Data(id)->text = text;
}

void Overlay::Font(ID id, int fontIndex)
{
	Data(id)->fontIndex = fontIndex;
}

void Overlay::Parent(ID id, ID parent)
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

	if (Behavior(id)->onTopOfParent)
	{
		uiPos[id].z = uiPos[parent].z + (1.0f / (numLayers * 10));
	}

}

void Overlay::Custom(ID id, CustomAttributes::Custom custom)
{
	uiCustom[id] = custom;
}

void Overlay::Rotation(ID id, float rotation)
{
	Look(id)->rotation = rotation;
}

void Overlay::SetWindowHandle(HWND window)
{
	this->window = window;
	RECT hwndRect;
	GetClientRect(this->window, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;
}

DirectX::XMFLOAT3 Overlay::Pos(ID id)
{
	if (id == ID::_NONE) return XMFLOAT3(0.0f, 0.0f, 0.0f);
	return uiPos[id];
}

Vec3 Overlay::RealPos(ID id)
{
	if(Parent(id) == ID::_NONE) return uiPos[id];
	return uiPos[id] + uiPos[uiParent[id]];
}

DirectX::XMFLOAT2 Overlay::Size(ID id)
{
	if (id == ID::_NONE) return XMFLOAT2(0.0f, 0.0f);
	return uiSize[id];
}

CA::Look* Overlay::Look(ID id)
{
	return &uiAttrib[id].look;
}

CA::Behavior* Overlay::Behavior(ID id)
{
	return &uiAttrib[id].behavior;
}

CA::Scaling* Overlay::Scaling(ID id)
{
	return &uiAttrib[id].scaling;
}

CA::Data* Overlay::Data(ID id)
{
	return &uiAttrib[id].data;
}

DirectX::SimpleMath::Color Overlay::Color(ID id)
{
	return uiAttrib[id].look.color;
}

int Overlay::Texture(ID id)
{
	return uiAttrib[id].look.texture;
}

std::string Overlay::Text(ID id)
{
	return Data(id)->text;
}

int Overlay::Font(ID id)
{
	return Data(id)->fontIndex;
}

ID Overlay::Parent(ID id)
{
	return uiParent[id];
}

CustomAttributes::Custom* Overlay::Custom(ID id)
{
	return &uiCustom[id];
}

DirectX::XMFLOAT2 Overlay::GetMousePos()
{
	return mousePos;
}

DirectX::XMFLOAT2 Overlay::GetDeltaMousePos()
{
	return deltaMousePos;
}

DirectX::XMFLOAT2 Overlay::GetRelativeClickPos()
{
	return relativeClickPos;
}

std::chrono::milliseconds Overlay::GetMouseLeftPressTime()
{
	return mouseLeftPressedTime;
}

std::chrono::milliseconds Overlay::GetMouseRightPressTime()
{
	return mouseRightPressedTime;
}

bool Overlay::MouseLeftPressed()
{
	return mouseLeftPressed;
}

bool Overlay::MouseRightPressed()
{
	return mouseRightPressed;
}

bool Overlay::MouseLeftLocked()
{
	return mouseLeftLocked;
}

bool Overlay::MouseRightLocked()
{
	return mouseRightLocked;
}

int Overlay::GetActiveID()
{
	return activeID;
}

int Overlay::GetHoverID()
{
	return hoveringOverID;
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