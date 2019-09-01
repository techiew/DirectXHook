#pragma once

#include <iostream>
#include <Windows.h>
#include <intrin.h>
#include <dxgi.h>
#include <d3d11.h>
#include <fstream>
#include <string>
#include "DebugConsole.h"
#include "Renderer.h"
#include "Mesh.h"
#include "TexturedBox.h"
#include "Textures.h"
#include <comdef.h>
#include "Text.h"
#include "Fonts.h"
#include <direct.h>
#include <stdio.h>

// We redo the defines from DllMain, because the defines are not global

// Check if 64 bit
#ifdef _WIN64
#define is64bit 1
#endif

/*
* A QWORD consists of 8 bytes in memory, and if we are dealing
* with a 64-bit program, where the memory addresses are 8 bytes long,
* we use QWORD (same size as unsigned __int64) to hold memory addresses.
* Otherwise, with 32-bit, we use a DWORD, which consists of 4 bytes
*/
#ifdef is64bit
#define JmpToHookAndJmpBack JmpToHookAndJmpBack64
typedef unsigned __int64 QWORD; // My C++ doesn't have QWORD for some reason
typedef QWORD MEMADDR; 
#else
#define JmpToHookAndJmpBack JmpToHookAndJmpBack
typedef DWORD MEMADDR;
#endif

// Type definition of the DXGI present function, so we can cast memory addresses
// to this function, this way we can call the function at the given memory address
// and pass parameters to the registers and stack at the location.
// 64-bit Present actually uses the __fastcall calling convention, but the compiler changes
// __stdcall to __fastcall automatically for 64-bit compilation.
typedef HRESULT (__stdcall* PresentFunction)(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags);

class Core
{
private:
	HMODULE originalDll;
	MEMADDR originalDllBaseAddress;
	MEMADDR originalPresentFunctionOffset;
	PresentFunction originalPresentFunction;
	bool texturesLoaded = false;
	bool meshesCreated = false;
	bool fontsLoaded = false;
	bool textCreated = false;
	bool drawExamples = false;
	bool showDebugConsole = false;
	std::vector<Mesh> thingsToDraw = std::vector<Mesh>();
	std::vector<Text> textToDraw = std::vector<Text>();
	std::shared_ptr<std::string> originalInstructionsBuffer;

public:
	MEMADDR newPresentReturn;
	DebugConsole console;
	Renderer renderer;
	Textures textures;
	Fonts fonts;

	void Init(HMODULE originalDll);
	void Hook(MEMADDR originalFunction, MEMADDR newFunction, int bytes);
	void Update();
	void Render(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	void AddMeshForDrawing(Mesh mesh);
	void AddTextForDrawing(Text text);
	~Core();
};