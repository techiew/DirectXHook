#include "DirectXHook.h"
#include "Overlays/PauseEldenRing/PauseEldenRing.h"

DirectXHook::DirectXHook()
{
	static PauseEldenRing pauseEldenRing;
	SetRenderCallback(&pauseEldenRing);
}

void DirectXHook::Hook()
{
	hookInstance = this;

	m_logger.Log("Hooking...");
	m_logger.Log("OnPresent: %p", &OnPresent);
	m_logger.Log("OnResizeBuffers: %p", &OnResizeBuffers);

	LoadLibrary("reshade.dll");

	// Let other hooks finish their business before we hook.
	// For some reason, sleeping causes MSI Afterburner to crash the application.
	Sleep(5000);
	if (IsDllLoaded("RTSSHooks64.dll"))
	{
		MessageBox(NULL, "DirectXHook is incompatible with MSI afterburner and RivaTuner Statistics Server. Please ensure they are closed and restart the game.", "Incompatible overlay", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
		return;
	}
	Sleep(25000);

	m_dummySwapChain = CreateDummySwapChain();
	HookSwapChainVmt(m_dummySwapChain, &originalPresentAddress, &originalResizeBuffersAddress, (uintptr_t)&OnPresent, (uintptr_t)&OnResizeBuffers);

	if (IsDllLoaded("d3d12.dll"))
	{
		m_dummyCommandQueue = CreateDummyCommandQueue();
		HookCommandQueueVmt(m_dummyCommandQueue, &originalExecuteCommandListsAddress, (uintptr_t)&OnExecuteCommandLists);
	}
}

void DirectXHook::DrawExampleTriangle(bool doDraw)
{
	renderer.DrawExampleTriangle(doDraw);
}

void DirectXHook::SetRenderCallback(IRenderCallback* object)
{
	renderer.SetRenderCallback(object);
}

bool DirectXHook::IsDllLoaded(std::string dllName)
{
	std::vector<HMODULE> modules(0, 0);
	DWORD lpcbNeeded;
	EnumProcessModules(GetCurrentProcess(), &modules[0], modules.size(), &lpcbNeeded);
	modules.resize(lpcbNeeded, 0);
	EnumProcessModules(GetCurrentProcess(), &modules[0], modules.size(), &lpcbNeeded);

	std::string lpBaseName (dllName.length(), 'x');
	for (auto module : modules)
	{
		GetModuleBaseName(GetCurrentProcess(), module, &lpBaseName[0], lpBaseName.length() + 1);
		if (lpBaseName == dllName)
		{
			m_logger.Log("%s is loaded", dllName);
			return true;
		}
	}

	return false;
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
		m_logger.Log("CreateDeviceAndSwapChain failed: %s", error.ErrorMessage());
		DestroyWindow(desc.OutputWindow);
		UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));
		return nullptr;
	}

	dummyDevice->Release();
	DestroyWindow(desc.OutputWindow);
	UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));

	m_logger.Log("CreateDeviceAndSwapChain succeeded");

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

	m_logger.Log("Command queue: %p", dummyCommandQueue);

	return dummyCommandQueue;
}

// Hooks the functions that we need in the Virtual Method Table of IDXGISwapChain.
// A pointer to the VMT of an object exists in the first 4/8 bytes of the object (or the last bytes, depending on the compiler).
void DirectXHook::HookSwapChainVmt(IDXGISwapChain* dummySwapChain, uintptr_t* originalPresentAddress, uintptr_t* originalResizeBuffersAddress, uintptr_t newPresentAddress, uintptr_t newResizeBuffersAddress)
{
	int size = sizeof(size_t);

	uintptr_t vmtBaseAddress = (*(uintptr_t*)dummySwapChain);
	uintptr_t vmtPresentIndex = (vmtBaseAddress + (size * 8));
	uintptr_t vmtResizeBuffersIndex = (vmtBaseAddress + (size * 13));

	m_logger.Log("SwapChain VMT base address: %p", vmtBaseAddress);
	m_logger.Log("SwapChain VMT Present index: %p", vmtPresentIndex);
	m_logger.Log("SwapChain VMT ResizeBuffers index: %p", vmtResizeBuffersIndex);

	DWORD oldProtection;
	DWORD oldProtection2;

	VirtualProtect((void*)vmtPresentIndex, size, PAGE_EXECUTE_READWRITE, &oldProtection);
	VirtualProtect((void*)vmtResizeBuffersIndex, size, PAGE_EXECUTE_READWRITE, &oldProtection2);

	*originalPresentAddress = (*(uintptr_t*)vmtPresentIndex);
	*originalResizeBuffersAddress = (*(uintptr_t*)vmtResizeBuffersIndex);

	// This sets the VMT entries to point towards our functions instead.
	*(uintptr_t*)vmtPresentIndex = newPresentAddress;
	*(uintptr_t*)vmtResizeBuffersIndex = newResizeBuffersAddress;

	VirtualProtect((void*)vmtPresentIndex, size, oldProtection, &oldProtection);
	VirtualProtect((void*)vmtResizeBuffersIndex, size, oldProtection2, &oldProtection2);

	dummySwapChain->Release();

	m_logger.Log("Original Present address: %p", originalPresentAddress);
	m_logger.Log("Original ResizeBuffers address: %p", originalResizeBuffersAddress);
}

void DirectXHook::HookCommandQueueVmt(ID3D12CommandQueue* dummyCommandQueue, uintptr_t* originalExecuteCommandListsAddress, uintptr_t newExecuteCommandListsAddress)
{
	uintptr_t vmtBaseAddress = (*(uintptr_t*)dummyCommandQueue);
	uintptr_t vmtExecuteCommandListsIndex = (vmtBaseAddress + (8 * 10));

	m_logger.Log("CommandQueue VMT base address: %p", vmtBaseAddress);
	m_logger.Log("ExecuteCommandLists index: %p", vmtExecuteCommandListsIndex);

	DWORD oldProtection;

	VirtualProtect((void*)vmtExecuteCommandListsIndex, 8, PAGE_EXECUTE_READWRITE, &oldProtection);

	*originalExecuteCommandListsAddress = (*(uintptr_t*)vmtExecuteCommandListsIndex);
	*(uintptr_t*)vmtExecuteCommandListsIndex = newExecuteCommandListsAddress;

	VirtualProtect((void*)vmtExecuteCommandListsIndex, 8, oldProtection, &oldProtection);

	m_logger.Log("Original ExecuteCommandLists address: %p", originalExecuteCommandListsAddress);
}
