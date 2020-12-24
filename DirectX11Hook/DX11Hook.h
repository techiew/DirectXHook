#pragma once

#include <d3d11.h>
#include <Windows.h>
#include <sysinfoapi.h>
#include <Psapi.h>
#include <cstdint>
#include "DebugConsole.h"
#include "Renderer.h"

typedef HRESULT (__stdcall* Present)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
typedef HRESULT (__stdcall* ResizeBuffers)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

class DX11Hook
{
public:
	DX11Hook();
	bool Hook();

private:
	static DebugConsole console;
	static Renderer renderer;
	IDXGISwapChain* dummySwapChain;
	static uintptr_t presentMiddleMan;
	static uintptr_t resizeBuffersMiddleMan;
	uintptr_t vmtBaseAddress;
	uintptr_t vmtPresentIndex;
	uintptr_t vmtResizeBuffersIndex;
	static uintptr_t originalPresentAddress;
	static uintptr_t originalResizeBuffersAddress;
	static bool firstRun;
	static bool fixApplied;

	bool CreateMiddleMan();
	bool CreateDummySwapChain();
	uintptr_t HookSwapChainVMT();
	static void FixDoubleHooks();
	static uintptr_t FindTrampolineDestination(uintptr_t trampJmp);
	HRESULT static __stdcall OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags);
	HRESULT static __stdcall OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
};