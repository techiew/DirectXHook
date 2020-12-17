#pragma once

#include <d3d11.h>
#include <Windows.h>
#include <Psapi.h>
#include <inttypes.h>
#include "DebugConsole.h"
#include "Renderer.h"

typedef HRESULT(__stdcall* PresentFunction)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffersFunction)(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

class DX11Hook
{
public:
	DX11Hook();
	bool Hook(HMODULE dxgiHandle);

private:
	static DebugConsole console;
	static Renderer renderer;
	void* vmtBaseAddress;
	void* vmtPresentIndex;
	void* vmtResizeBuffersIndex;
	static void* originalPresentAddress;
	static void* originalResizeBuffersAddress;
	static bool firstRun;
	IDXGISwapChain* dummySwapChain;

	bool CreateDummySwapChain();
	bool HookSwapChainVMT();
	HRESULT static __stdcall OnPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	HRESULT static __stdcall OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
};