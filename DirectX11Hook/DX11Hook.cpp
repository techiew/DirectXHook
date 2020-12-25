#include "DX11Hook.h"

// Static members
DebugConsole DX11Hook::console = DebugConsole();
Renderer DX11Hook::renderer = Renderer();
uintptr_t DX11Hook::originalPresentAddress = 0;
uintptr_t DX11Hook::originalResizeBuffersAddress = 0;
uintptr_t DX11Hook::presentMiddleMan = 0;
uintptr_t DX11Hook::resizeBuffersMiddleMan = 0;
bool DX11Hook::firstRun = true;
bool DX11Hook::fixApplied = false;

DX11Hook::DX11Hook()
{
	console.Open();
	renderer.DrawExamples(true);
	firstRun = true;
}

bool DX11Hook::Hook()
{
	console.Print(MsgType::STARTPROCESS, "Hooking...");

	CreateMiddleMan();
	CreateDummySwapChain();
	HookSwapChainVMT();

	return true;
}

// Creates a space between the VMT and the original function we want to hook
// It's kind of a blend between VMT hooking and hooking with trampolines.
// I found this to be a good solution for compatibility with other hooks.
// I'm not sure if this technique already has a name, but this is what I call it.
bool DX11Hook::CreateMiddleMan()
{
	int size = 28; // Arbitrary

	presentMiddleMan = (uintptr_t)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	resizeBuffersMiddleMan = (uintptr_t)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (presentMiddleMan == 0 || resizeBuffersMiddleMan == 0)
	{
		console.Print(MsgType::FAILED, "Failed to allocate memory for the hook!");
		return false;
	}

	memset((void*)presentMiddleMan, 0x90, size);
	memset((void*)resizeBuffersMiddleMan, 0x90, size);

	console.Print("Allocated memory at: %p", presentMiddleMan);
	console.Print("Allocated memory at: %p", resizeBuffersMiddleMan);

#ifdef _WIN64
	*(uintptr_t*)(presentMiddleMan + 14) = 0x00000000000025FF;
	*(uintptr_t*)(presentMiddleMan + 14 + 6) = (uintptr_t)&OnPresent;
	*(uintptr_t*)(resizeBuffersMiddleMan + 14) = 0x00000000000025FF;
	*(uintptr_t*)(resizeBuffersMiddleMan + 14 + 6) = (uintptr_t)&OnResizeBuffers;
#else
	*(uintptr_t*)(presentMiddleMan + 14) = 0x000000EA;
	*(uintptr_t*)(presentMiddleMan + 14 + 1) = (uintptr_t)&OnPresent;
	*(uintptr_t*)(resizeBuffersMiddleMan + 14) = 0x000000EA;
	*(uintptr_t*)(resizeBuffersMiddleMan + 14 + 1) = (uintptr_t)&OnResizeBuffers;
#endif

	return true;
}

// Creates an instance of the IDXGISwapChain class, so that we can use the object
// to gain access to the Virtual Method Table of the class.
bool DX11Hook::CreateDummySwapChain()
{
	WNDCLASSEX wc { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = TEXT("dummy class");

	RegisterClassExA(&wc);

	// We need to set up a dummy window for our device and swap chain creation.
	// I don't know the exact reason, but if we create another device for the main window, everything crashes.
	HWND hWnd = CreateWindow(wc.lpszClassName, TEXT(""), WS_DISABLED, 0, 0, 0, 0, NULL, NULL, NULL, nullptr);

	DXGI_SWAP_CHAIN_DESC desc{ 0 };
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1;
	desc.OutputWindow = hWnd; // Note that we are using our dummy window here
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

	HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, featureLevel, 1, D3D11_SDK_VERSION, &desc, &dummySwapChain, &dummyDevice, NULL, NULL);

	if (FAILED(result))
	{
		_com_error error(result);
		console.Print(MsgType::FAILED, "CreateDeviceAndSwapChain failed: %s", error.ErrorMessage());
		DestroyWindow(desc.OutputWindow);
		UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));
		return false;
	}

	dummyDevice->Release();

	DestroyWindow(desc.OutputWindow);
	UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));

	console.Print("CreateDeviceAndSwapChain succeeded");

	return true;
}

// Gets the Virtual Method Table for the IDXGISwapChain class and hooks the functions we need
// A pointer to the VMT of an object exists in the first 4/8 bytes of the object. (Or the last bytes, depends on the compiler)
uintptr_t DX11Hook::HookSwapChainVMT()
{
	int size = sizeof(size_t);

#ifdef _WIN64
	int vmtPresentOffset = 64;
	int vmtResizeOffset = 98;
#else
	int vmtPresentOffset = 216;
	int vmtResizeOffset = 236;
#endif

	// Here we find the correct entries in the VMT for both functions
	vmtBaseAddress = (*(uintptr_t*)dummySwapChain);
	vmtPresentIndex = (vmtBaseAddress + (size * 8)); // * 8
	vmtResizeBuffersIndex = (vmtBaseAddress + (size * 13)); // * 13

	console.Print("VMT base address: %p", vmtBaseAddress);
	console.Print("VMT Present index: %p", vmtPresentIndex);
	console.Print("VMT ResizeBuffers index: %p", vmtResizeBuffersIndex);

	DWORD oldProtection;
	DWORD oldProtection2;

	VirtualProtect((void*)vmtPresentIndex, size, PAGE_EXECUTE_READWRITE, &oldProtection);
	VirtualProtect((void*)vmtResizeBuffersIndex, size, PAGE_EXECUTE_READWRITE, &oldProtection2);

	originalPresentAddress = (*(uintptr_t*)vmtPresentIndex);
	originalResizeBuffersAddress = (*(uintptr_t*)vmtResizeBuffersIndex);

	// This sets the VMT entries to point towards our functions instead.
	*(uintptr_t*)vmtPresentIndex = presentMiddleMan;
	*(uintptr_t*)vmtResizeBuffersIndex = resizeBuffersMiddleMan;

	VirtualProtect((void*)vmtPresentIndex, size, oldProtection, &oldProtection);
	VirtualProtect((void*)vmtResizeBuffersIndex, size, oldProtection2, &oldProtection2);

	dummySwapChain->Release();

	console.Print("Original Present address: %p", originalPresentAddress);
	console.Print("Original ResizeBuffers address: %p", originalResizeBuffersAddress);

	return true;
}

// This fixes potential issues with other hooks, such as the Steam overlay, which 
// can hook themselves both in our middle man and the original function at the same time.
// We need to check for this issue when OnPresent/OnResizeBuffers runs, as that 
// is the moment the double hook can occur.
void DX11Hook::FixDoubleHooks()
{
	if (fixApplied) return;

	unsigned char jmpBytePresent = *(unsigned char*)originalPresentAddress;
	unsigned char jmpByteResize = *(unsigned char*)originalResizeBuffersAddress;
	unsigned char jmpByteMiddlePresent = *(unsigned char*)presentMiddleMan;
	unsigned char jmpByteMiddleResize = *(unsigned char*)resizeBuffersMiddleMan;

	uintptr_t trampDst1;
	uintptr_t trampDst2;

	DWORD oldProtection;
	DWORD oldProtection2;

	VirtualProtect((void*)originalPresentAddress, 8, PAGE_EXECUTE_READWRITE, &oldProtection);
	VirtualProtect((void*)originalResizeBuffersAddress, 8, PAGE_EXECUTE_READWRITE, &oldProtection2);

#ifdef _WIN64
	std::vector<unsigned char> presentOrigBytes = { 0x48, 0x89, 0x5C, 0x24, 0x10 };
	std::vector<unsigned char> resizeOrigBytes = { 0x48, 0x8B, 0xC4, 0x55, 0x41, 0x54 };
#else
	std::vector<unsigned char> presentOrigBytes = { 0x8B, 0xFF, 0x55, 0x8B, 0xEC };
	std::vector<unsigned char> resizeOrigBytes = { 0x68, 0xA4, 0x00, 0x00, 0x00 };
#endif

	if (jmpBytePresent == 0xE9 && jmpByteMiddlePresent == 0xE9)
	{
		trampDst1 = FindTrampolineDestination(originalPresentAddress);
		trampDst2 = FindTrampolineDestination(presentMiddleMan);

		console.Print("pTrampDst1: %p", trampDst1);
		console.Print("pTrampDst2: %p", trampDst2);

		if (trampDst1 == trampDst2)
		{
			memcpy((void*)originalPresentAddress, &presentOrigBytes[0], presentOrigBytes.size());
		}

	}

	if (jmpByteResize == 0xE9 && jmpByteMiddleResize == 0xE9)
	{
		trampDst1 = FindTrampolineDestination(originalResizeBuffersAddress);
		trampDst2 = FindTrampolineDestination(resizeBuffersMiddleMan);

		console.Print("rTrampDst1: %p", trampDst1);
		console.Print("rTrampDst2: %p", trampDst2);

		if (trampDst1 == trampDst2)
		{
			memcpy((void*)originalResizeBuffersAddress, &resizeOrigBytes[0], resizeOrigBytes.size());
		}

	}

	VirtualProtect((void*)originalPresentAddress, 8, oldProtection, &oldProtection);
	VirtualProtect((void*)originalResizeBuffersAddress, 8, oldProtection2, &oldProtection2);

	fixApplied = true;
}

// Finds the final destination of a trampoline placed by other hooks,
// this lets us determine the owner of the trampoline.
uintptr_t DX11Hook::FindTrampolineDestination(uintptr_t firstJmpAddr)
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
#ifdef _WIN64
		memcpy(&offset, (void*)(absolute + 1), 4);
		destination = absolute + offset + 5;
#else
		memcpy(&destination, (void*)(absolute + 1), 4);
#endif
	}

	return destination;
}

/*
* The real Present will get hooked and then detour to this function.
* Present is part of the final rendering stage in DirectX.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
*/
HRESULT __stdcall DX11Hook::OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags)
{

	if (firstRun)
	{
		console.Print("Greetings from the hooked Present!");
		FixDoubleHooks();
		firstRun = false;
	}

	renderer.OnPresent(pThis, syncInterval, flags);

	return ((Present)originalPresentAddress)(pThis, syncInterval, flags);
}

/*
* The real ResizeBuffers will get hooked and then detour to this function.
* ResizeBuffers usually gets called when the window resizes (not all games call it).
* We need to hook this so we can release our reference to the render target when it's called.
* If we don't do this then the game will most likely crash.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
*/
HRESULT __stdcall DX11Hook::OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	console.Print("ResizeBuffers was called!");

	FixDoubleHooks();

	renderer.OnResizeBuffers(pThis, bufferCount, width, height, newFormat, swapChainFlags);

	return ((ResizeBuffers)originalResizeBuffersAddress)(pThis, bufferCount, width, height, newFormat, swapChainFlags);
}
