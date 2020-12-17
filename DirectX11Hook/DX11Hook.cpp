#include "DX11Hook.h"

// Static members
DebugConsole DX11Hook::console = DebugConsole();
Renderer DX11Hook::renderer = Renderer();
void* DX11Hook::originalPresentAddress = nullptr;
void* DX11Hook::originalResizeBuffersAddress = nullptr;
bool DX11Hook::firstRun = true;

DX11Hook::DX11Hook()
{
	console.Open();
	renderer.DrawExamples(true);
}

bool DX11Hook::Hook(HMODULE dxgiHandle)
{
	console.Print("Hooking...", MsgType::STARTPROCESS);

	MODULEINFO gorInfo;
	GetModuleInformation(GetCurrentProcess(), dxgiHandle, &gorInfo, sizeof(MODULEINFO));
	console.Print("dxgi.dll base address: %p", gorInfo.lpBaseOfDll);

	if (CreateDummySwapChain() && HookSwapChainVMT())
	{
		console.Print("Hooking completed successfully", MsgType::COMPLETE);
		return true;
	}
	else
	{
		console.Print("Critical Error: Something went wrong while hooking!", MsgType::FAILED);
		return false;
	}

}

// Creates an instance of the IDXGISwapChain class, which we won't use for it's intended purpose,
// rather we use it to gain access to the pointer that lets us find its Virtual Method Table.
bool DX11Hook::CreateDummySwapChain()
{
	WNDCLASSEX wc{ 0 };
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
		console.Print("CreateDeviceAndSwapChain failed: %s", (void*)error.ErrorMessage(), MsgType::FAILED);
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
bool DX11Hook::HookSwapChainVMT()
{ 

#ifdef _WIN64
	int length = 8;
#else
	int length = 4;
#endif

	vmtBaseAddress = (uint64_t*)(*(uint64_t*)dummySwapChain);
	vmtPresentIndex = (uint64_t*)((uint64_t)vmtBaseAddress + (length * 8));
	vmtResizeBuffersIndex = (uint64_t*)((uint64_t)vmtBaseAddress + (length * 13)); // 205

	console.Print("VMT base address: %p", vmtBaseAddress);
	console.Print("VMT Present index: %p", vmtPresentIndex);
	console.Print("VMT ResizeBuffers index: %p", vmtResizeBuffersIndex);

	DWORD oldProtection;
	DWORD oldProtection2;

	VirtualProtect(vmtPresentIndex, length, PAGE_EXECUTE_READWRITE, &oldProtection);
	VirtualProtect(vmtResizeBuffersIndex, length, PAGE_EXECUTE_READWRITE, &oldProtection2);

	originalPresentAddress = (uint64_t*)(*(uint64_t*)vmtPresentIndex);
	originalResizeBuffersAddress = (uint64_t*)(*(uint64_t*)vmtResizeBuffersIndex);

	// Here we change the VMT entries to point towards our functions instead.
	void* temp1 = &OnPresent;
	void* temp2 = &OnResizeBuffers;
	memcpy(vmtPresentIndex, &temp1, length);
	memcpy(vmtResizeBuffersIndex, &temp2, length);

	VirtualProtect(vmtPresentIndex, length, oldProtection, &oldProtection);
	VirtualProtect(vmtResizeBuffersIndex, length, oldProtection2, &oldProtection2);

	dummySwapChain->Release();

	console.Print("Original Present address: %p", originalPresentAddress);
	console.Print("Original ResizeBuffers address: %p", originalResizeBuffersAddress);

	return true;
}

HRESULT __stdcall DX11Hook::OnPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{

	if (firstRun)
	{
		console.Print("Greetings from the hooked Present!");
		firstRun = false;
	}

	renderer.OnPresent(swapChain, syncInterval, flags);

	return ((PresentFunction)originalPresentAddress)(swapChain, syncInterval, flags);
}

HRESULT __stdcall DX11Hook::OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	console.Print("ResizeBuffers was called!");

	renderer.OnResizeBuffers(bufferCount, width, height, newFormat, swapChainFlags);

	return ((ResizeBuffersFunction)originalResizeBuffersAddress)(bufferCount, width, height, newFormat, swapChainFlags);
}
