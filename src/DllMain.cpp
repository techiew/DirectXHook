#include <Windows.h>

#include "DirectXHook.h"
#include "Logger.h"
#include "MemUtils.h"
#include "Example/Example.h"

FARPROC forwardExports[6];
static Logger logger{ "DllMain" };

HMODULE LoadDllFromSystemFolder(std::string dllName)
{
	std::string systemFolderPath = "";
	char dummy[1];
	UINT pathLength = GetSystemDirectoryA(dummy, 1);
	systemFolderPath.resize(pathLength);
	LPSTR lpSystemFolderPath = const_cast<char*>(systemFolderPath.c_str());
	GetSystemDirectoryA(lpSystemFolderPath, systemFolderPath.size());
	systemFolderPath = lpSystemFolderPath;

	logger.Log("System folder path: %s", systemFolderPath.c_str());

	HMODULE dll = LoadLibraryA(std::string(systemFolderPath + "\\" + dllName).c_str());
	return dll;
}

bool CreateDinputForwardExports(HMODULE dll)
{
	if (!dll)
	{
		return false;
	}

	forwardExports[0] = GetProcAddress(dll, "DirectInput8Create");
	forwardExports[1] = GetProcAddress(dll, "DllCanUnloadNow");
	forwardExports[2] = GetProcAddress(dll, "DllGetClassObject");
	forwardExports[3] = GetProcAddress(dll, "DllRegisterServer");
	forwardExports[4] = GetProcAddress(dll, "DllUnregisterServer");
	forwardExports[5] = GetProcAddress(dll, "GetdfDIJoystick");

	for (int i = 0; i < 6; i++)
	{
		if (forwardExports[i] == NULL)
		{
			logger.Log("Could not forward export function %i", i);
			return false;
		}
	}

	return true;
}

void OpenDebugTerminal()
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
}

DWORD WINAPI HookThread(LPVOID lpParam)
{
	HMODULE originalDll = LoadDllFromSystemFolder("dinput8.dll");
	if (!CreateDinputForwardExports(originalDll))
	{
		logger.Log("Failed to set up forward exports");
		return 1;
	}

	static Renderer renderer;
	static DirectXHook dxHook(&renderer);
	static Example example;
	dxHook.AddRenderCallback(&example);
	dxHook.Hook();
	return 0;
}

extern "C"
{
	int AsmJmp();
	FARPROC address = NULL;
	void JumpToFunction(unsigned int index)
	{
		address = forwardExports[index];
		AsmJmp();
	}
	void PROXY_DirectInput8Create()
	{
		JumpToFunction(0);
	}
	void PROXY_DllCanUnloadNow()
	{
		JumpToFunction(1);
	}
	void PROXY_DllGetClassObject()
	{
		JumpToFunction(2);
	}
	void PROXY_DllRegisterServer()
	{
		JumpToFunction(3);
	}
	void PROXY_DllUnregisterServer()
	{
		JumpToFunction(4);
	}
	void PROXY_GetdfDIJoystick()
	{
		JumpToFunction(5);
	}
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		OpenDebugTerminal();
		CreateThread(0, 0, &HookThread, 0, 0, NULL);
	}

	return 1;
}