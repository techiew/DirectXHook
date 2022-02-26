#include "DirectXHook.h"

// Static members
std::string DirectXHook::m_printPrefix = "DirectXHook >";
Renderer DirectXHook::m_renderer;
uintptr_t DirectXHook::m_presentTrampoline = 0;
uintptr_t DirectXHook::m_resizeBuffersTrampoline = 0;
uintptr_t DirectXHook::m_executeCommandListsTrampoline = 0;
uintptr_t DirectXHook::m_originalPresentAddress = 0;
uintptr_t DirectXHook::m_originalResizeBuffersAddress = 0;
uintptr_t DirectXHook::m_originalExecuteCommandListsAddress = 0;
std::vector<std::vector<unsigned char>> DirectXHook::m_functionHeaders;
bool DirectXHook::m_firstPresent = true;
bool DirectXHook::m_firstResizeBuffers = true;
bool DirectXHook::m_firstExecuteCommandLists = true;

DirectXHook::DirectXHook()
{
	std::fstream terminalEnableFile;
	terminalEnableFile.open("hook_enable_terminal.txt", std::fstream::in);
	if (terminalEnableFile.is_open())
	{
		if (AllocConsole())
		{
			freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
			SetWindowText(GetConsoleWindow(), "DirectXHook");
		}
		terminalEnableFile.close();
	}

	SetFunctionHeaders();
}

void DirectXHook::Hook()
{
	printf("%s Hooking...\n", m_printPrefix);

	Sleep(5000); // Helps against double hooks

	m_presentTrampoline = CreateBufferedTrampoline(&OnPresent);
	m_resizeBuffersTrampoline = CreateBufferedTrampoline(&OnResizeBuffers);
	m_dummySwapChain = CreateDummySwapChain();
	HookSwapChainVmt(m_dummySwapChain, m_presentTrampoline, m_resizeBuffersTrampoline);

	if (IsDirectX12Loaded())
	{
		m_dummyCommandQueue = CreateDummyCommandQueue();
		m_executeCommandListsTrampoline = CreateBufferedTrampoline(&OnExecuteCommandLists);
		HookCommandQueueVmt(m_dummyCommandQueue, m_executeCommandListsTrampoline);
	}
}

void DirectXHook::DrawExamples(bool doDraw)
{
	m_renderer.DrawExamples(doDraw);
}

void DirectXHook::SetRenderCallback(IRenderCallback* object)
{
	m_renderer.SetRenderCallback(object);
}

bool DirectXHook::IsDirectX12Loaded()
{
	std::vector<HMODULE> modules(0, 0);
	DWORD lpcbNeeded;
	EnumProcessModules(GetCurrentProcess(), &modules[0], modules.size(), &lpcbNeeded);
	modules.resize(lpcbNeeded, 0);
	EnumProcessModules(GetCurrentProcess(), &modules[0], modules.size(), &lpcbNeeded);

	std::string lpBaseName = "XXXXXXXXX";
	for (auto module : modules)
	{
		GetModuleBaseName(GetCurrentProcess(), module, &lpBaseName[0], 10);
		if (lpBaseName == std::string("d3d12.dll"))
		{
			printf("%s DirectX 12 is loaded\n", m_printPrefix);
			return true;
		}
	}

	return false;
}

// Basically creates a trampoline with a buffer of NOP's before the jump.
// The buffer lets other hooks place their jumps at the top without screwing with our stuff.
uintptr_t DirectXHook::CreateBufferedTrampoline(void* destination)
{
	int size = 28; // Arbitrary
	uintptr_t trampoline = (uintptr_t)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (trampoline == 0)
	{
		printf("%s Failed to allocate memory for the hook!\n", m_printPrefix);
		return 0;
	}

	memset((void*)trampoline, 0x90, size);

	printf("%s Allocated memory at: %p\n", m_printPrefix, trampoline);

#ifdef _WIN64
	*(uintptr_t*)(trampoline + 14) = 0x00000000000025FF;
	*(uintptr_t*)(trampoline + 14 + 6) = (uintptr_t)destination;
#else
	*(uintptr_t*)(trampoline + 14) = 0x000000E9;
	*(uintptr_t*)(trampoline + 14 + 1) = (uintptr_t)destination - (trampoline + 14) - 5;
#endif

	return trampoline;
}

IDXGISwapChain* DirectXHook::CreateDummySwapChain()
{
	WNDCLASSEX wc { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = TEXT("dummy class");
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindow(wc.lpszClassName, TEXT(""), WS_DISABLED, 0, 0, 0, 0, NULL, NULL, NULL, nullptr);

	DXGI_SWAP_CHAIN_DESC desc{ 0 };
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1;
	desc.OutputWindow = hWnd;
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
	HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, featureLevel, 1, D3D11_SDK_VERSION, &desc, &dummySwapChain, &dummyDevice, NULL, NULL);
	if (FAILED(result))
	{
		_com_error error(result);
		printf("%s CreateDeviceAndSwapChain failed: %s\n", m_printPrefix, error.ErrorMessage());
		DestroyWindow(desc.OutputWindow);
		UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));
		return nullptr;
	}

	dummyDevice->Release();
	DestroyWindow(desc.OutputWindow);
	UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));

	printf("%s CreateDeviceAndSwapChain succeeded\n", m_printPrefix);

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

	printf("%s Command queue: %p\n", m_printPrefix, dummyCommandQueue);

	return dummyCommandQueue;
}

// Hooks the functions that we need in the Virtual Method Table of IDXGISwapChain.
// A pointer to the VMT of an object exists in the first 4/8 bytes of the object (or the last bytes, depending on the compiler).
void DirectXHook::HookSwapChainVmt(IDXGISwapChain* dummySwapChain, uintptr_t newPresentAddress, uintptr_t newResizeBuffersAddress)
{
	int size = sizeof(size_t);

	uintptr_t vmtBaseAddress = (*(uintptr_t*)dummySwapChain);
	uintptr_t vmtPresentIndex = (vmtBaseAddress + (size * 8));
	uintptr_t vmtResizeBuffersIndex = (vmtBaseAddress + (size * 13));

	printf("%s SwapChain VMT base address: %p\n", m_printPrefix, vmtBaseAddress);
	printf("%s SwapChain VMT Present index: %p\n", m_printPrefix, vmtPresentIndex);
	printf("%s SwapChain VMT ResizeBuffers index: %p\n", m_printPrefix, vmtResizeBuffersIndex);

	DWORD oldProtection;
	DWORD oldProtection2;

	VirtualProtect((void*)vmtPresentIndex, size, PAGE_EXECUTE_READWRITE, &oldProtection);
	VirtualProtect((void*)vmtResizeBuffersIndex, size, PAGE_EXECUTE_READWRITE, &oldProtection2);

	m_originalPresentAddress = (*(uintptr_t*)vmtPresentIndex);
	m_originalResizeBuffersAddress = (*(uintptr_t*)vmtResizeBuffersIndex);

	// This sets the VMT entries to point towards our functions instead.
	*(uintptr_t*)vmtPresentIndex = newPresentAddress;
	*(uintptr_t*)vmtResizeBuffersIndex = newResizeBuffersAddress;

	VirtualProtect((void*)vmtPresentIndex, size, oldProtection, &oldProtection);
	VirtualProtect((void*)vmtResizeBuffersIndex, size, oldProtection2, &oldProtection2);

	dummySwapChain->Release();

	printf("%s Original Present address: %p\n", m_printPrefix, m_originalPresentAddress);
	printf("%s Original ResizeBuffers address: %p\n", m_printPrefix, m_originalResizeBuffersAddress);
}

void DirectXHook::HookCommandQueueVmt(ID3D12CommandQueue* dummyCommandQueue, uintptr_t newExecuteCommandListsAddress)
{
	uintptr_t vmtBaseAddress = (*(uintptr_t*)dummyCommandQueue);
	uintptr_t vmtExecuteCommandListsIndex = (vmtBaseAddress + (8 * 10));

	printf("%s CommandQueue VMT base address: %p\n", m_printPrefix, vmtBaseAddress);
	printf("%s ExecuteCommandLists index: %p\n", m_printPrefix, vmtExecuteCommandListsIndex);

	DWORD oldProtection;

	VirtualProtect((void*)vmtExecuteCommandListsIndex, 8, PAGE_EXECUTE_READWRITE, &oldProtection);

	m_originalExecuteCommandListsAddress = (*(uintptr_t*)vmtExecuteCommandListsIndex);
	*(uintptr_t*)vmtExecuteCommandListsIndex = newExecuteCommandListsAddress;

	VirtualProtect((void*)vmtExecuteCommandListsIndex, 8, oldProtection, &oldProtection);

	printf("%s Original ExecuteCommandLists address: %p\n", m_printPrefix, m_originalExecuteCommandListsAddress);
}

void DirectXHook::SetFunctionHeaders()
{
	m_functionHeaders.push_back({ 0x48, 0x89, 0x5C, 0x24, 0x10 }); // Present
	m_functionHeaders.push_back({ 0x48, 0x8B, 0xC4, 0x55, 0x41, 0x54 }); // ResizeBuffers
	m_functionHeaders.push_back({ 0x48, 0x89, 0x5C, 0x24, 0x08 }); // ExecuteCommandLists
}

// This is used to fix potential issues with other hooks such as the Steam overlay, which
// can place jumps in our trampolines and in the original function at the same time.
void DirectXHook::RemoveDoubleHooks(uintptr_t trampolineAddress, uintptr_t originalFunctionAddress, std::vector<unsigned char> originalFunctionHeader)
{
	unsigned char firstByteAtTrampoline = *(unsigned char*)trampolineAddress;
	unsigned char firstByteAtOriginal = *(unsigned char*)originalFunctionAddress;

	DWORD oldProtection;
	VirtualProtect((void*)originalFunctionAddress, originalFunctionHeader.size(), PAGE_EXECUTE_READWRITE, &oldProtection);

	if (firstByteAtTrampoline == 0xE9 && firstByteAtOriginal == 0xE9)
	{
		uintptr_t trampolineDestination = FindTrampolineDestination(trampolineAddress);
		uintptr_t trampolineDestination2 = FindTrampolineDestination(originalFunctionAddress);

		if (trampolineDestination == trampolineDestination2)
		{
			memcpy((void*)originalFunctionAddress, &originalFunctionHeader[0], originalFunctionHeader.size());
			printf("%s Removed double hook at %p\n", m_printPrefix, trampolineAddress);
		}
	}

	VirtualProtect((void*)originalFunctionAddress, originalFunctionHeader.size(), oldProtection, &oldProtection);
}

// Finds the final destination of a trampoline placed by other hooks.
uintptr_t DirectXHook::FindTrampolineDestination(uintptr_t firstJmpAddr)
{
	int offset;
	uintptr_t absolute;
	uintptr_t destination;

	memcpy(&offset, (void*)(firstJmpAddr + 1), 4);
	absolute = firstJmpAddr + offset + 5;

	if (*(unsigned char*)absolute == 0xFF)
	{
		memcpy(&destination, (void*)(absolute + 6), 8);
	} 
	else if (*(unsigned char*)absolute == 0xE9)
	{
		memcpy(&offset, (void*)(absolute + 1), 4);
		destination = absolute + offset + 5;
	}

	return destination;
}

/*
* The real Present will get hooked and then detour to this function.
* Present is part of the final rendering stage in DirectX.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
*/
HRESULT __stdcall DirectXHook::OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags)
{
	if (m_firstPresent)
	{
		RemoveDoubleHooks(m_presentTrampoline, m_originalPresentAddress, m_functionHeaders[0]);
		m_firstPresent = false;
	}

	m_renderer.OnPresent(pThis, syncInterval, flags);
	return ((Present)m_originalPresentAddress)(pThis, syncInterval, flags);
}

/*
* The real ResizeBuffers will get hooked and then detour to this function.
* ResizeBuffers usually gets called when the window resizes.
* We need to hook this so we can release our reference to the render target when it's called.
* If we don't do this then the game will most likely crash.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
*/
HRESULT __stdcall DirectXHook::OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	if (m_firstResizeBuffers)
	{
		RemoveDoubleHooks(m_resizeBuffersTrampoline, m_originalResizeBuffersAddress, m_functionHeaders[1]);
		m_firstResizeBuffers = false;
	}

	m_renderer.OnResizeBuffers(pThis, bufferCount, width, height, newFormat, swapChainFlags);
	return ((ResizeBuffers)m_originalResizeBuffersAddress)(pThis, bufferCount, width, height, newFormat, swapChainFlags);
}

/* 
* The real ExecuteCommandLists will get hooked and then detour to this function.
* ExecuteCommandLists gets called when work is to be submitted to the GPU.
* We need to hook this to grab the command queue so we can use it to create the D3D11On12 device in DirectX 12 games.
*/
void __stdcall DirectXHook::OnExecuteCommandLists(ID3D12CommandQueue* pThis, UINT numCommandLists, const ID3D12CommandList** ppCommandLists)
{
	if (m_firstExecuteCommandLists)
	{
		RemoveDoubleHooks(m_executeCommandListsTrampoline, m_originalExecuteCommandListsAddress, m_functionHeaders[2]);
		m_firstExecuteCommandLists = false;
	}

	if (m_renderer.missingCommandQueue && pThis->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
		m_renderer.SetCommandQueue(pThis);
	}

	((ExecuteCommandLists)m_originalExecuteCommandListsAddress)(pThis, numCommandLists, ppCommandLists);
}
