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
#include "CustomElements.h"
#include "CustomAttributes.h"

using namespace std::chrono;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace CustomElements;

class Overlay
{
private:
	// Functions for CustomOverlay to implement
	// If you want custom functions with access to class members, add them here
	void Load();
	void Draw();
	void ReadValues();
	void CheckHotkeys();
	void DoStuff();

private:
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	DebugConsole* console;
	HWND window;
	Textures textures;
	Fonts fonts;
	int windowWidth = 0;
	int windowHeight = 0;

	CustomElements::Tex tex;
	CustomElements::Font font;
	CustomElements::ID id;

	// UI state
	std::vector<DirectX::XMFLOAT3> uiPos = std::vector<DirectX::XMFLOAT3>();
	std::vector<DirectX::XMFLOAT2> uiSize = std::vector<DirectX::XMFLOAT2>();
	std::vector<std::shared_ptr<Attributes>> uiAttrib = std::vector<std::shared_ptr<Attributes>>();
	std::vector<int> configSignups = std::vector<int>();

	// Mouse state
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(0.0f, 0.0f);
	DirectX::XMFLOAT2 deltaMousePos = DirectX::XMFLOAT2(0.0f, 0.0f);
	std::chrono::milliseconds mouseLeftPressedTime;
	std::chrono::milliseconds mouseRightPressedTime;
	bool mouseLeftPressed = false;
	bool mouseRightPressed = false;
	bool mouseLocked = false;
	int activeID = 0;
	int hoveringOverID = 0;
	int stopMoreClicksOnID = 0;
	int numLayers = 100;

	// Functions that run upon initialization
	void Signup(ID id);
	void ReadConfig();
	void SortByDepth();

	// Rendering functions
	bool DoBox(ID id, int texIndex, ID parent);
	bool DoBox(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, int texIndex, Attributes attrib);
	//bool DoBox(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, int texIndex, float rotation = 0.0f, DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f), bool moveable = true, bool clickable = true, DirectX::SpriteEffects spriteEffects = DirectX::SpriteEffects_None);
	bool DoText(ID id, int fontIndex, ID parent);
	bool DoText(const char* text, DirectX::XMFLOAT3 pos, float size, int fontIndex, Attributes attrib);
	//bool DoText(const char* text, DirectX::XMFLOAT3 pos, float size, int fontIndex, float rotation = 0.0f, DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f));
	//void DoBezierCurve();

	// Logic functions for checking for mouse clicks etc.
	void CheckMouseState();
	bool IsMouseInside(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT2 area);
	bool CheckElementClicked(int element);
	void OnClick(int element);
	void DragElementWithMouse(int element);

	// Conversion
	DirectX::XMFLOAT2 ToPixels(float ndcX, float ndcY);
	DirectX::XMFLOAT2 ToNDC(float pixelsX, float pixelsY);
	DirectX::SimpleMath::Color RGBAColor(float r, float g, float b, float a);
	float MapNumberInRange(float number, float inputStart, float inputEnd, float outputStart, float outputEnd);

	// Setters
	void SetDef(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size);
	void SetDef(ID id, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size, Attributes attrib);
	void SetPos(ID id, DirectX::XMFLOAT3 pos);
	void SetSize(ID id, DirectX::XMFLOAT2 size);
	void SetAttrib(ID id, Attributes attrib);
	void SetRotation(ID id, float rotation);
	void SetColor(ID id, DirectX::SimpleMath::Color color);

	// Getters
	DirectX::XMFLOAT3 GetPos(ID id);
	DirectX::XMFLOAT2 GetSize(ID id);
	Attributes* GetAttrib(ID id);
	DirectX::XMFLOAT2 GetMousePos();
	DirectX::XMFLOAT2 GetDeltaMousePos();
	std::chrono::milliseconds GetMouseLeftPressTime();
	std::chrono::milliseconds GetMouseRightPressTime();
	bool IsMouseLeftPressed();
	bool IsMouseRightPressed();
	bool IsMouseLockedOnElement();
	int GetActiveID();
	int GetHoveringOverID();
	int GetStopClicksOnID();
	int GetWindowWidth();
	int GetWindowHeight();
	std::chrono::milliseconds GetTimeMs();

public:
	Overlay() {};
	Overlay(ID3D11Device* device, DirectX::SpriteBatch* spriteBatch, DebugConsole* console, HWND window);
	void Update();
	void SetWindowHandle(HWND window);
};