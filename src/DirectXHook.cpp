#include "DirectXHook.h"
#include "RiseDpsMeter/RiseDpsMeter.h"
#include "Example/Example.h"
#include "PauseTheGame/PauseTheGame.h"

static DirectXHook* hookInstance = nullptr;

/*
* Present will get hooked and detour to this function.
* Present is part of the final rendering stage in DirectX.
* We need to hook this so we can grab the pointer to the game's swapchain and use it to render.
* We also place our own rendering code within this function call.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
*/
HRESULT __stdcall OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags)
{
	hookInstance->renderer->OnPresent(pThis, syncInterval, flags);
	return ((Present)hookInstance->presentReturnAddress)(pThis, syncInterval, flags);
}

/*
* ResizeBuffers will get hooked and detour to this function.
* ResizeBuffers usually gets called by the game when the window resizes.
* We need to hook this so we can release our references to various resources when it's called.
* If we don't do this then the game will most likely crash.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
*/
HRESULT __stdcall OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	hookInstance->renderer->OnResizeBuffers(pThis, bufferCount, width, height, newFormat, swapChainFlags);
	return ((ResizeBuffers)hookInstance->resizeBuffersReturnAddress)(pThis, bufferCount, width, height, newFormat, swapChainFlags);
}

/*
* ExecuteCommandLists will get hooked and detour to this function.
* ExecuteCommandLists gets called when work is to be submitted to the GPU.
* We need to hook this so we can grab the command queue and use it to create the D3D11On12 device in DirectX 12 games.
* https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists
*/
void __stdcall OnExecuteCommandLists(ID3D12CommandQueue* pThis, UINT numCommandLists, const ID3D12CommandList** ppCommandLists)
{
	static bool commandQueueRetrieved = false;
	if (pThis->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT && !commandQueueRetrieved)
	{
		hookInstance->renderer->SetCommandQueue(pThis);
		commandQueueRetrieved = true;
	}

	((ExecuteCommandLists)hookInstance->executeCommandListsReturnAddress)(pThis, numCommandLists, ppCommandLists);
}

DirectXHook::DirectXHook(ID3DRenderer* renderer)
{
	this->renderer = renderer;
	hookInstance = this;
}

void DirectXHook::Hook()
{
	logger.Log("OnPresent: %p", &OnPresent);
	logger.Log("OnResizeBuffers: %p", &OnResizeBuffers);

	CreateDummyDeviceAndSwapChain();
	HookSwapChain(dummySwapChain.Get(), (uintptr_t)&OnPresent, (uintptr_t)&OnResizeBuffers, &presentReturnAddress, &resizeBuffersReturnAddress);

	CreateDummyCommandQueue();
	HookCommandQueue(dummyCommandQueue.Get(), (uintptr_t)&OnExecuteCommandLists, &hookInstance->executeCommandListsReturnAddress);

	SafelyReleaseDummyResources();
}

void DirectXHook::AddRenderCallback(IRenderCallback* object)
{
	renderer->AddRenderCallback(object);
}

void DirectXHook::CreateDummyDeviceAndSwapChain()
{
	WNDCLASSEX wc { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = TEXT("dummy class");
	RegisterClassExA(&wc);
	HWND hwnd = CreateWindow(wc.lpszClassName, TEXT(""), WS_DISABLED, 0, 0, 0, 0, NULL, NULL, NULL, nullptr);

	DXGI_SWAP_CHAIN_DESC desc{ 0 };
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1;
	desc.OutputWindow = hwnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1
	};

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		NULL, 
		D3D_DRIVER_TYPE_HARDWARE,
		NULL, 
		0, 
		featureLevels, 
		1,
		D3D11_SDK_VERSION,
		&desc, 
		dummySwapChain.GetAddressOf(),
		dummyD3D11Device.GetAddressOf(), 
		NULL, 
		NULL);

	DestroyWindow(desc.OutputWindow);
	UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));

	if (FAILED(result))
	{
		_com_error error(result);
		logger.Log("D3D11CreateDeviceAndSwapChain failed: %s", error.ErrorMessage());
		return;
	}

	logger.Log("D3D11CreateDeviceAndSwapChain succeeded");
}

void DirectXHook::CreateDummyD3D12Device()
{
	D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(dummyD3D12Device.GetAddressOf()));
}

void DirectXHook::CreateDummyCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	CreateDummyD3D12Device();

	dummyD3D12Device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(dummyCommandQueue.GetAddressOf()));

	logger.Log("Command queue: %p", dummyCommandQueue);
}

void DirectXHook::HookSwapChain(
	IDXGISwapChain* dummySwapChain,
	uintptr_t presentDetourFunction,
	uintptr_t resizeBuffersDetourFunction,
	uintptr_t* presentReturnAddress,
	uintptr_t* resizeBuffersReturnAddress)
{
	int vmtPresentOffset = 8;
	int vmtResizeBuffersOffset = 13;
	size_t numBytes = sizeof(size_t);

	uintptr_t vmtBaseAddress = (*(uintptr_t*)dummySwapChain);
	uintptr_t vmtPresentIndex = (vmtBaseAddress + (numBytes * vmtPresentOffset));
	uintptr_t vmtResizeBuffersIndex = (vmtBaseAddress + (numBytes * vmtResizeBuffersOffset));

	logger.Log("SwapChain VMT base address: %p", vmtBaseAddress);
	logger.Log("SwapChain VMT Present index: %p", vmtPresentIndex);
	logger.Log("SwapChain VMT ResizeBuffers index: %p", vmtResizeBuffersIndex);

	MemoryUtils::ToggleMemoryProtection(false, vmtPresentIndex, numBytes);
	MemoryUtils::ToggleMemoryProtection(false, vmtResizeBuffersIndex, numBytes);

	uintptr_t presentAddress = (*(uintptr_t*)vmtPresentIndex);
	uintptr_t resizeBuffersAddress = (*(uintptr_t*)vmtResizeBuffersIndex);

	logger.Log("Present address: %p", presentAddress);
	logger.Log("ResizeBuffers address: %p", resizeBuffersAddress);

	MemoryUtils::ToggleMemoryProtection(true, vmtPresentIndex, numBytes);
	MemoryUtils::ToggleMemoryProtection(true, vmtResizeBuffersIndex, numBytes);

	MemoryUtils::PlaceHook(presentAddress, presentDetourFunction, presentReturnAddress);
	MemoryUtils::PlaceHook(resizeBuffersAddress, resizeBuffersDetourFunction, resizeBuffersReturnAddress);
}

void DirectXHook::HookCommandQueue(
	ID3D12CommandQueue* dummyCommandQueue,
	uintptr_t executeCommandListsDetourFunction,
	uintptr_t* executeCommandListsReturnAddress)
{
	int vmtExecuteCommandListsOffset = 10;
	size_t numBytes = 8;

	uintptr_t vmtBaseAddress = (*(uintptr_t*)dummyCommandQueue);
	uintptr_t vmtExecuteCommandListsIndex = (vmtBaseAddress + (numBytes * vmtExecuteCommandListsOffset));

	logger.Log("CommandQueue VMT base address: %p", vmtBaseAddress);
	logger.Log("ExecuteCommandLists index: %p", vmtExecuteCommandListsIndex);

	MemoryUtils::ToggleMemoryProtection(false, vmtExecuteCommandListsIndex, numBytes);
	executeCommandListsAddress = (*(uintptr_t*)vmtExecuteCommandListsIndex);
	MemoryUtils::ToggleMemoryProtection(true, vmtExecuteCommandListsIndex, numBytes);

	logger.Log("ExecuteCommandLists address: %p", executeCommandListsAddress);

	bool hookIsPresent = MemoryUtils::IsAddressHooked(executeCommandListsAddress);
	if (hookIsPresent)
	{
		logger.Log("Hook already present in ExecuteCommandLists");
	}

	MemoryUtils::PlaceHook(executeCommandListsAddress, executeCommandListsDetourFunction, executeCommandListsReturnAddress);
}

void DirectXHook::SafelyReleaseDummyResources()
{
	bool isRtssLoaded =
		MemoryUtils::IsDllLoaded("RTSSHooks64.dll")
		|| MemoryUtils::IsDllLoaded("RTSSHooks.dll");
	if (isRtssLoaded)
	{
		// Don't release our dummy resources as RTSS will cause a crash.
		// This only happens if this hook is injected (not proxied) between
		// 5-10 seconds after the game has booted.
		// I think RTSS hooks D3D11CreateDeviceAndSwapChain and grabs
		// the dummy resources we create. This causes a null pointer error 
		// if we release them.
		logger.Log("RTSS is loaded");
	}
	else
	{
		// Not releasing the swapchain and device will with some games 
		// increase the GPU wattage quite dramatically, even if they 
		// are not actually in use. However it does not seem to increase
		// GPU USAGE or affect performance.
		dummySwapChain->Release();
		dummyCommandQueue->Release();
		dummyD3D11Device->Release();
		dummyD3D12Device->Release();
	}
}