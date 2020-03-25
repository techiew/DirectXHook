#pragma once

#include <SpriteBatch.h>
#include <d3d11.h>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <functional>
#include <SimpleMath.h>
#include "DebugConsole.h"
#include "Fonts.h"
#include "Textures.h"
#include "Attributes.h"
#include "CustomAttributes.h"
#include "CustomElements.h"
#include "MemReader.h"
#include "SigScanner.h"

using namespace std::chrono;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace CustomElements;
namespace CA = CoreAttributes;

typedef XMFLOAT3 Vec3;
typedef XMFLOAT2 Vec2;

class Overlay
{
private:
	// Functions for CustomOverlay.cpp to implement
	// If you need new functions with access to class members, add them here
	void Load();
	void Draw();
	bool GetAddresses();
	void ReadValues();
	void CheckHotkeys();
	void DoStuff();

private:
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	DebugConsole* console;
	HWND window;
	Textures textures;
	Fonts fonts;
	SigScanner scanner;
	MemReader mem;
	int windowWidth = 0;
	int windowHeight = 0;

	// Instances of CustomElements structs
	CustomElements::Texture tex;
	CustomElements::Font font;
	CustomElements::ID id;

	// UI state
	std::vector<DirectX::XMFLOAT3> uiPos = std::vector<DirectX::XMFLOAT3>();
	std::vector<DirectX::XMFLOAT2> uiSize = std::vector<DirectX::XMFLOAT2>();
	std::vector<CA::Attributes> uiAttrib = std::vector<CA::Attributes>();
	std::vector<CustomAttributes::Custom> uiCustom = std::vector<CustomAttributes::Custom>();
	std::vector<ID> uiParent = std::vector<ID>();
	std::vector<std::vector<bool>> uiChildren = std::vector<std::vector<bool>>();
	std::vector<int> numChildren = std::vector<int>();
	std::vector<int> configSignups = std::vector<int>();

	// Mouse state
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(0.0f, 0.0f);
	DirectX::XMFLOAT2 deltaMousePos = DirectX::XMFLOAT2(0.0f, 0.0f);
	DirectX::XMFLOAT2 relativeClickPos = DirectX::XMFLOAT2(0.0f, 0.0f);
	std::chrono::milliseconds mouseLeftPressedTime;
	std::chrono::milliseconds mouseRightPressedTime;
	bool mouseLeftPressed = false;
	bool mouseRightPressed = false;
	bool mouseLeftLocked = false;
	bool mouseRightLocked = false;
	int activeID = 0;
	int clickingID = 0;
	int hoveringOverID = 0;
	int numLayers = 100;
	float edgeMargin = 10;

	// Misc functions
	void Signup(ID id);
	void ReadConfig();
	void SortByDepth();
	void PlaceElementOnTop(ID id);

	// Rendering functions
	bool DoBox(ID id);
	bool DoBox(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size); // For when you don't care about using the config file
	bool DoBox(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, int texIndex, CA::Attributes attrib); // For when you don't care about the ID system
	bool DoText(ID id);
	bool DoText(const char* text, DirectX::XMFLOAT3 pos, float size, int fontIndex, CA::Attributes attrib);
	//bool DoBox(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, int texIndex, float rotation = 0.0f, DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f), bool moveable = true, bool clickable = true, DirectX::SpriteEffects spriteEffects = DirectX::SpriteEffects_None);
	//bool DoText(const char* text, DirectX::XMFLOAT3 pos, float size, int fontIndex, float rotation = 0.0f, DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f));
	//void DoBezierCurve();

	// Logic functions for checking for mouse clicks etc.
	void CheckMouseState();
	bool CheckElementClicked(int element);
	void OnLeftClick(int element);
	void OnRightClick(int element);
	void DragOrResizeElement(int element);
	bool IsMouseInside(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 area);

	// Collision detection
	XMFLOAT2 IsFullyInsideArea(XMFLOAT3 pos, XMFLOAT2 size, XMFLOAT3 areaPos, XMFLOAT2 areaSize);

	// Conversion
	DirectX::XMFLOAT2 NDC(float ndcX, float ndcY);
	DirectX::XMFLOAT2 PixelsToNDC(float pixelsX, float pixelsY);
	DirectX::SimpleMath::Color RGBA(float r, float g, float b, float a);
	float MapNumberInRange(float number, float inputStart, float inputEnd, float outputStart, float outputEnd);

	// Setters
	void PosSize(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size);
	void Pos(ID id, DirectX::XMFLOAT3 pos);
	void Size(ID id, DirectX::XMFLOAT2 size);
	void Look(ID id, CA::Look look);
	void Behavior(ID id, CA::Behavior behavior);
	void Scaling(ID id, CA::Scaling scaling);
	void Rotation(ID id, float rotation);
	void Color(ID id, float r, float g, float b, float a);
	void Texture(ID id, int texIndex);
	void Text(ID id, std::string text);
	void Font(ID id, int fontIndex);
	void Parent(ID id, ID parent);
	void Custom(ID id, CustomAttributes::Custom custom);

	// Getters
	DirectX::XMFLOAT3 Pos(ID id);
	Vec3 RealPos(ID id);
	DirectX::XMFLOAT2 Size(ID id);
	CA::Look* Look(ID id);
	CA::Behavior* Behavior(ID id);
	CA::Scaling* Scaling(ID id);
	CA::Data* Data(ID id);
	DirectX::SimpleMath::Color Color(ID id);
	int Texture(ID id);
	std::string Text(ID id);
	int Font(ID id);
	ID Parent(ID id);
	CustomAttributes::Custom* Custom(ID id);
	DirectX::XMFLOAT2 GetMousePos();
	DirectX::XMFLOAT2 GetDeltaMousePos();
	DirectX::XMFLOAT2 GetRelativeClickPos();
	std::chrono::milliseconds GetMouseLeftPressTime();
	std::chrono::milliseconds GetMouseRightPressTime();
	bool MouseLeftPressed();
	bool MouseRightPressed();
	bool MouseLeftLocked();
	bool MouseRightLocked();
	int GetActiveID();
	int GetHoverID();
	int GetWindowWidth();
	int GetWindowHeight();
	std::chrono::milliseconds GetTimeMs();

public:
	Overlay() {};
	Overlay(ID3D11Device* device, DirectX::SpriteBatch* spriteBatch, DebugConsole* console, HWND window);
	void Render();
	void SetWindowHandle(HWND window);
};