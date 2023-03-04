#include <Windows.h>

#include "DirectXHook.h"

HMODULE originalDll = 0;
FARPROC originalFunctions[21];

DWORD WINAPI HookThread(LPVOID lpParam)
{
	std::string systemPath = "";
	char dummy[1];
	UINT pathLength = GetSystemDirectoryA(dummy, 1);
	systemPath.resize(pathLength);
	LPSTR lpSystemPath = const_cast<char*>(systemPath.c_str());
	GetSystemDirectoryA(lpSystemPath, systemPath.size());
	systemPath = lpSystemPath;
	originalDll = LoadLibraryA(std::string(systemPath + "\\dxgi.dll").c_str());
	if (originalDll)
	{
		// Set function addresses we need for forward exporting
		originalFunctions[0] = GetProcAddress(originalDll, "ApplyCompatResolutionQuirking");
		originalFunctions[1] = GetProcAddress(originalDll, "CompatString");
		originalFunctions[2] = GetProcAddress(originalDll, "CompatValue");
		originalFunctions[3] = GetProcAddress(originalDll, "CreateDXGIFactory");
		originalFunctions[4] = GetProcAddress(originalDll, "CreateDXGIFactory1");
		originalFunctions[5] = GetProcAddress(originalDll, "CreateDXGIFactory2");
		originalFunctions[6] = GetProcAddress(originalDll, "DXGID3D10CreateDevice");
		originalFunctions[7] = GetProcAddress(originalDll, "DXGID3D10CreateLayeredDevice");
		originalFunctions[8] = GetProcAddress(originalDll, "DXGID3D10ETWRundown");
		originalFunctions[9] = GetProcAddress(originalDll, "DXGID3D10GetLayeredDeviceSize");
		originalFunctions[10] = GetProcAddress(originalDll, "DXGID3D10RegisterLayers");
		originalFunctions[11] = GetProcAddress(originalDll, "DXGIDeclareAdapterRemovalSupport");
		originalFunctions[12] = GetProcAddress(originalDll, "DXGIDumpJournal");
		originalFunctions[13] = GetProcAddress(originalDll, "DXGIGetDebugInterface1");
		originalFunctions[14] = GetProcAddress(originalDll, "DXGIReportAdapterConfiguration");
		originalFunctions[15] = GetProcAddress(originalDll, "DXGIRevertToSxS");
		originalFunctions[16] = GetProcAddress(originalDll, "PIXBeginCapture");
		originalFunctions[17] = GetProcAddress(originalDll, "PIXEndCapture");
		originalFunctions[18] = GetProcAddress(originalDll, "PIXGetCaptureState");
		originalFunctions[19] = GetProcAddress(originalDll, "SetAppCompatStringPointer");
		originalFunctions[20] = GetProcAddress(originalDll, "UpdateHMDEmulationStatus");
	}
	else
	{
		return false;
	}

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

	static DirectXHook dxHook;
	dxHook.Hook();
	return S_OK;
}

extern "C"
{
	int AsmJmp();
	FARPROC address = NULL;
	void JumpToOriginalFunction(unsigned int index)
	{
		address = originalFunctions[index];
		AsmJmp();
	}
	void PROXY_ApplyCompatResolutionQuirking()
	{
		JumpToOriginalFunction(0);
	}
	void PROXY_CompatString()
	{
		JumpToOriginalFunction(1);
	}
	void PROXY_CompatValue()
	{
		JumpToOriginalFunction(2);
	}
	void PROXY_CreateDXGIFactory()
	{
		JumpToOriginalFunction(3);
	}
	void PROXY_CreateDXGIFactory1()
	{
		JumpToOriginalFunction(4);
	}
	void PROXY_CreateDXGIFactory2()
	{
		JumpToOriginalFunction(5);
	}
	void PROXY_DXGID3D10CreateDevice()
	{
		JumpToOriginalFunction(6);
	}
	void PROXY_DXGID3D10CreateLayeredDevice()
	{
		JumpToOriginalFunction(7);
	}
	void PROXY_DXGID3D10ETWRundown()
	{
		JumpToOriginalFunction(8);
	}
	void PROXY_DXGID3D10GetLayeredDeviceSize()
	{
		JumpToOriginalFunction(9);
	}
	void PROXY_DXGID3D10RegisterLayers()
	{
		JumpToOriginalFunction(10);
	}
	void PROXY_DXGIDeclareAdapterRemovalSupport()
	{
		JumpToOriginalFunction(11);
	}
	void PROXY_DXGIDumpJournal()
	{
		JumpToOriginalFunction(12);
	}
	void PROXY_DXGIGetDebugInterface1()
	{
		JumpToOriginalFunction(13);
	}
	void PROXY_DXGIReportAdapterConfiguration()
	{
		JumpToOriginalFunction(14);
	}
	void PROXY_DXGIRevertToSxS()
	{
		JumpToOriginalFunction(15);
	}
	void PROXY_PIXBeginCapture()
	{
		JumpToOriginalFunction(16);
	}
	void PROXY_PIXEndCapture()
	{
		JumpToOriginalFunction(17);
	}
	void PROXY_PIXGetCaptureState()
	{
		JumpToOriginalFunction(18);
	}
	void PROXY_SetAppCompatStringPointer()
	{
		JumpToOriginalFunction(19);
	}
	void PROXY_UpdateHMDEmulationStatus()
	{
		JumpToOriginalFunction(20);
	}
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(module);
		CreateThread(0, 0, &HookThread, 0, 0, NULL);
	}

	return 1;
}