#pragma once

#include <d3d11.h>
#include <d3d12.h>
#include <Windows.h>
#include <Psapi.h>

#include "Renderer.h"
#include "IRenderCallback.h"
#include "Logger.h"

class DirectXHook
{
public:
	Renderer renderer;
	uintptr_t originalPresentAddress = 0;
	uintptr_t originalResizeBuffersAddress = 0;
	uintptr_t originalExecuteCommandListsAddress = 0;

	DirectXHook();
	void Hook();
	void DrawExampleTriangle(bool doDraw);
	void SetRenderCallback(IRenderCallback* object);
private:
	Logger m_logger{ "DirectXHook" };
	IDXGISwapChain* m_dummySwapChain = nullptr;
	ID3D12CommandQueue* m_dummyCommandQueue = nullptr;
	std::vector<std::vector<unsigned char>> m_functionHeaders;

	bool IsDllLoaded(std::string dllName);
	IDXGISwapChain* CreateDummySwapChain();
	ID3D12CommandQueue* CreateDummyCommandQueue();
	void HookSwapChainVmt(IDXGISwapChain* dummySwapChain, uintptr_t* originalPresentAddress, uintptr_t* originalResizeBuffersAddress, uintptr_t newPresentAddress, uintptr_t newResizeBuffersAddress);
	void HookCommandQueueVmt(ID3D12CommandQueue* dummyCommandQueue, uintptr_t* originalExecuteCommandListsAddress, uintptr_t newExecuteCommandListsAddress);
};

static DirectXHook* hookInstance = nullptr;

/*
* Here we have typedefs of the functions we want to hook.
* We have these so we can call these functions through pointers to their memory addresses.
*
* Setting the proper calling convention is important (__stdcall).
* It makes it so function arguments are written/read to/from memory in the correct way.
* 64-bit functions actually use the __fastcall calling convention, but the compiler changes
* __stdcall to __fastcall automatically for 64-bit compilation.
*/
typedef HRESULT(__stdcall* Present)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef void(__stdcall* ExecuteCommandLists)(ID3D12CommandQueue* This, UINT NumCommandLists, const ID3D12CommandList** ppCommandLists);

/*
* The real Present will get hooked and then detour to this function.
* Present is part of the final rendering stage in DirectX.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
*/
inline HRESULT __stdcall OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags)
{
	hookInstance->renderer.OnPresent(pThis, syncInterval, flags);
	return ((Present)hookInstance->originalPresentAddress)(pThis, syncInterval, flags);
}

/*
* The real ResizeBuffers will get hooked and then detour to this function.
* ResizeBuffers usually gets called when the window resizes.
* We need to hook this so we can release our reference to the render target when it's called.
* If we don't do this then the game will most likely crash.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
*/
inline HRESULT __stdcall OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	hookInstance->renderer.OnResizeBuffers(pThis, bufferCount, width, height, newFormat, swapChainFlags);
	return ((ResizeBuffers)hookInstance->originalResizeBuffersAddress)(pThis, bufferCount, width, height, newFormat, swapChainFlags);
}

/*
* The real ExecuteCommandLists will get hooked and then detour to this function.
* ExecuteCommandLists gets called when work is to be submitted to the GPU.
* We need to hook this to grab the command queue so we can use it to create the D3D11On12 device in DirectX 12 games.
*/
inline void __stdcall OnExecuteCommandLists(ID3D12CommandQueue* pThis, UINT numCommandLists, const ID3D12CommandList** ppCommandLists)
{
	if (hookInstance->renderer.missingCommandQueue && pThis->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
		hookInstance->renderer.SetCommandQueue(pThis);
	}

	((ExecuteCommandLists)hookInstance->originalExecuteCommandListsAddress)(pThis, numCommandLists, ppCommandLists);
}