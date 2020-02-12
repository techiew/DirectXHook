#pragma once

#include <iostream>
#include <Windows.h>
#include <intrin.h>
#include <dxgi.h>
#include <d3d11.h>
#include <fstream>
#include <string>
#include <comdef.h>
#include <direct.h>
#include <stdio.h>
#include "DebugConsole.h"
#include "Renderer.h"
#include "Textures.h"
#include "Fonts.h"
#include "SigScanner.h"

/*
* MEMADDR helps us to handle the pointers in the Hook function more cleanly.
* I found it leads to less casting than if we didn't use it.
* Also it makes the 32-bit and 64-bit hooking more similar.
*/
#ifdef _WIN64
typedef unsigned __int64 QWORD; 
typedef QWORD MEMADDR; 
#else
typedef DWORD MEMADDR;
#endif

/* 
* Here we have type definitions of the functions we want to hook. 
* We do this so we can treat pointers as actual functions, in other words
* we can call the function that the pointer is pointing to and pass 
* parameters to them aswell, just like a normal function call.
* 
* Setting the proper calling convention is important (__stdcall).
* It makes it so we can send function parameters into memory in the correct way.
* 64-bit functions actually use the __fastcall calling convention, but the compiler changes
* __stdcall to __fastcall automatically for 64-bit compilation.
* A great resource on calling conventions: https://www.codeproject.com/Articles/1388/Calling-Conventions-Demystified
*/
typedef HRESULT (__stdcall* PresentFunction)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffersFunction)(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

class Core
{
private:
	bool drawExamples = false;
	bool showDebugConsole = false;
	bool steamOverlayActive = false;
	SigScanner scanner;

	HMODULE originalDll;
	MEMADDR targetDllBaseAddress;
	MEMADDR targetPresentOffset;
	MEMADDR targetResizeBuffersOffset;
	PresentFunction* targetPresentFunction;
	ResizeBuffersFunction* targetResizeBuffersFunction;
	int presentHookSize;
	int resizeBuffersHookSize;
	const char* steamDllName;

	std::vector<void*> allocatedMemory = std::vector<void*>();

	void AllocateMemory(void** storePointer, int size);

public:
	MEMADDR newPresentReturn;
	MEMADDR newResizeBuffersReturn;
	DebugConsole console;
	Renderer renderer;

	void Init(HMODULE originalDll);
	ResizeBuffersFunction* FindResizeBuffersAddress(bool hookSteamOverlay);
	PresentFunction* FindPresentAddress(bool hookSteamOverlay);
	void Hook(void* hookFrom, void* hookTo, void* returnAddress, int bytes);
	void Update();
	void OnPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	void OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	~Core();
};