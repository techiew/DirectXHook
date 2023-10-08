#include <Windows.h>

#include "DirectXHook.h"
#include "Logger.h"
#include "MemoryUtils.h"
#include "Example/Example.h"
#include "UniversalProxyDLL.h"

static Logger logger{ "DllMain" };

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
	static Renderer renderer;
	static DirectXHook dxHook(&renderer);
	static Example example;
	dxHook.AddRenderCallback(&example);
	dxHook.Hook();
	return 0;
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		OpenDebugTerminal();
		UPD::MuteLogging();
		UPD::CreateProxy(module);
		CreateThread(0, 0, &HookThread, 0, 0, NULL);
	}

	return 1;
}