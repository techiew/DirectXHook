#include "DirectXHook.h"
#include "RiseDpsMeter/RiseDpsMeter.h"
#include "Example/Example.h"
#include "PauseTheGame/PauseTheGame.h"

static DirectXHook* hookInstance = nullptr;

/* 
* Note: The non-member functions in this file are defined as such because
* you are not allowed to pass around pointers to member functions.
*/

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
	if (pThis->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
		hookInstance->renderer->SetCommandQueue(pThis);
		//hookInstance->UnhookCommandQueue();
	}

	((ExecuteCommandLists)hookInstance->executeCommandListsReturnAddress)(pThis, numCommandLists, ppCommandLists);
}

void GetCommandQueue()
{
	ID3D12CommandQueue* dummyCommandQueue = hookInstance->CreateDummyCommandQueue();
	hookInstance->HookCommandQueue(dummyCommandQueue, (uintptr_t)&OnExecuteCommandLists, &hookInstance->executeCommandListsReturnAddress);
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

	renderer->SetGetCommandQueueCallback(&GetCommandQueue);
	IDXGISwapChain* dummySwapChain = CreateDummySwapChain();
	HookSwapChain(dummySwapChain, (uintptr_t)&OnPresent, (uintptr_t)&OnResizeBuffers, &presentReturnAddress, &resizeBuffersReturnAddress);
}

void DirectXHook::SetDrawExampleTriangle(bool doDraw)
{
	((Renderer*)renderer)->SetDrawExampleTriangle(doDraw);
}

void DirectXHook::AddRenderCallback(IRenderCallback* object)
{
	renderer->AddRenderCallback(object);
}

IDXGISwapChain* DirectXHook::CreateDummySwapChain()
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

	D3D_FEATURE_LEVEL featureLevel[] =
	{
		D3D_FEATURE_LEVEL_9_1,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1
	};

	ID3D11Device* dummyDevice;
	IDXGISwapChain* dummySwapChain;
	HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr, 
		D3D_DRIVER_TYPE_HARDWARE, 
		NULL, 
		NULL, 
		featureLevel, 
		1,
		D3D11_SDK_VERSION,
		&desc, 
		&dummySwapChain,
		&dummyDevice, 
		NULL, 
		NULL);

	DestroyWindow(desc.OutputWindow);
	UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));
	dummyDevice->Release();

	if (FAILED(result))
	{
		_com_error error(result);
		logger.Log("D3D11CreateDeviceAndSwapChain failed: %s", error.ErrorMessage());
		dummySwapChain->Release();
		return nullptr;
	}

	logger.Log("D3D11CreateDeviceAndSwapChain succeeded");
	return dummySwapChain;
}

ID3D12CommandQueue* DirectXHook::CreateDummyCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ID3D12Device* d12Device = nullptr;
	D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)&d12Device);

	ID3D12CommandQueue* dummyCommandQueue;
	d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&dummyCommandQueue));

	logger.Log("Command queue: %p", dummyCommandQueue);

	return dummyCommandQueue;
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

	dummySwapChain->Release();
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
	dummyCommandQueue->Release();
}

void DirectXHook::UnhookCommandQueue()
{
	MemoryUtils::Unhook(executeCommandListsAddress);
}