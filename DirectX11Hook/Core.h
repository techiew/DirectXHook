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
#include "SigScanner.h"

#ifdef _WIN64
typedef unsigned __int64 QWORD; 
typedef QWORD MEMADDR; 
#else
typedef DWORD MEMADDR;
#endif

/* 
* A type definition of the DXGI present function, so we can cast memory addresses
* to this function, this way we can call the function at the given memory address
* and pass parameters to the registers and stack at the location in the correct way.
* 64-bit Present actually uses the __fastcall calling convention, but the compiler changes
* __stdcall to __fastcall automatically for 64-bit compilation.
* A great resource on calling conventions: https://www.codeproject.com/Articles/1388/Calling-Conventions-Demystified
*/
typedef HRESULT (__stdcall* PresentFunction)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffersFunction)(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

class Core
{
private:
	HMODULE originalDll;
	MEMADDR targetDllBaseAddress;
	MEMADDR targetPresentOffset;
	MEMADDR targetResizeBuffersOffset;
	PresentFunction* targetPresentFunction;
	ResizeBuffersFunction* targetResizeBuffersFunction;
	bool drawExamples = false;
	bool showDebugConsole = false;
	bool steamOverlayActive = false;
	int presentHookSize;
	int resizeBuffersHookSize;
	const char* steamDllName;

	std::vector<Mesh> thingsToDraw = std::vector<Mesh>();
	std::vector<Text> textToDraw = std::vector<Text>();

	std::vector<void*> allocatedMemory = std::vector<void*>();
	SigScanner scanner;

	void AllocateMemory(void** storePointer, int size);

public:
	MEMADDR newPresentReturn;
	MEMADDR newResizeBuffersReturn;
	DebugConsole console;
	Renderer renderer;

	void Init(HMODULE originalDll);
	PresentFunction* FindPresentAddress(bool hookSteamOverlay);
	ResizeBuffersFunction* FindResizeBuffersAddress(bool hookSteamOverlay);
	void Hook(void* hookFrom, void* hookTo, void* returnAddress, int bytes);
	void Update();
	void OnPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	void OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	~Core();
};