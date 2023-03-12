#pragma once

#include <d3d11.h>
#include <d3d12.h>
#include <Windows.h>

#include "Renderer.h"
#include "ID3DRenderer.h"
#include "IRenderCallback.h"
#include "Logger.h"
#include "MemoryUtils.h"

/*
* Here we have typedefs of the functions we want to hook.
* They are defined so we can call the respective functions through pointers to their memory addresses.
*
* Setting the proper calling convention is important (__stdcall).
* It makes it so the function arguments are read/written to/from memory in the correct way.
* 64-bit functions actually use the __fastcall calling convention, but the compiler changes
* __stdcall to __fastcall automatically for 64-bit compilation.
*/
typedef HRESULT(__stdcall* Present)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef void(__stdcall* ExecuteCommandLists)(ID3D12CommandQueue* This, UINT NumCommandLists, const ID3D12CommandList** ppCommandLists);

// Hooks DirectX 11 and DirectX 12
class DirectXHook
{
public:
	ID3DRenderer* renderer;
	uintptr_t executeCommandListsAddress = 0;
	uintptr_t presentReturnAddress = 0;
	uintptr_t resizeBuffersReturnAddress = 0;
	uintptr_t executeCommandListsReturnAddress = 0;

	DirectXHook(ID3DRenderer* renderer);
	void Hook();
	void SetDrawExampleTriangle(bool doDraw);
	void AddRenderCallback(IRenderCallback* object);
	ID3D12CommandQueue* CreateDummyCommandQueue();
	void HookCommandQueue(ID3D12CommandQueue* dummyCommandQueue, uintptr_t executeCommandListsDetourFunction, uintptr_t* executeCommandListsReturnAddress);
	void UnhookCommandQueue();

private:
	Logger logger{ "DirectXHook" };

	IDXGISwapChain* CreateDummySwapChain();
	void HookSwapChain(IDXGISwapChain* dummySwapChain, uintptr_t presentDetourFunction, uintptr_t resizeBuffersDetourFunction, uintptr_t* presentReturnAddress, uintptr_t* resizeBuffersReturnAddress);
};