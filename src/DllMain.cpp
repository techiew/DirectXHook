#include <Windows.h>

#include "DirectXHook.h"
#include "Logger.h"
#include "MemoryUtils.h"
#include "Example/Example.h"
#include <locale>
#include <codecvt>

FARPROC forwardExports[6];
static Logger logger{ "DllMain" };

typedef HMODULE(__stdcall* LoadLibraryA_T)(LPCSTR lpLibFileName);
typedef HMODULE(__stdcall* LoadLibraryW_T)(LPCWSTR lpLibFileName);
typedef HMODULE(__stdcall* LoadLibraryExA_T)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(__stdcall* LoadLibraryExW_T)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

uintptr_t HookLoadLibraryAReturnAddress = 0;
uintptr_t HookLoadLibraryWReturnAddress = 0;
uintptr_t HookLoadLibraryExAReturnAddress = 0;
uintptr_t HookLoadLibraryExWReturnAddress = 0;

std::vector<std::string> blacklistedLibraries = {
	"RTSSHooks64.dll",
	"RTSSHooks.dll"
};

std::string GetLibraryNameWithoutPath(std::string libraryName)
{
	std::size_t search = libraryName.find_last_of("\\");
	if (search != std::string::npos)
	{
		return libraryName.substr(search + 1);
	}
	return libraryName;
}

bool IsLibraryBlacklisted(std::string libraryName)
{
	if (std::find(blacklistedLibraries.begin(), blacklistedLibraries.end(), libraryName) != blacklistedLibraries.end())
	{
		return true;
	}
	return false;
}

HMODULE __stdcall HookLoadLibraryA(LPCSTR lpLibFileName)
{
	if (IsLibraryBlacklisted(GetLibraryNameWithoutPath(lpLibFileName)))
	{
		SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return NULL;
	}
	return ((LoadLibraryA_T)HookLoadLibraryAReturnAddress)(lpLibFileName);
}

HMODULE __stdcall HookLoadLibraryW(LPCWSTR lpLibFileName)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wideStringConverter;
	if (IsLibraryBlacklisted(GetLibraryNameWithoutPath(wideStringConverter.to_bytes(lpLibFileName))))
	{
		SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return NULL;
	}
	return ((LoadLibraryW_T)HookLoadLibraryWReturnAddress)(lpLibFileName);
}

HMODULE __stdcall HookLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	if (IsLibraryBlacklisted(GetLibraryNameWithoutPath(lpLibFileName)))
	{
		SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return NULL;
	}
	return ((LoadLibraryExA_T)HookLoadLibraryExAReturnAddress)(lpLibFileName, hFile, dwFlags);
}

HMODULE __stdcall HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wideStringConverter;
	if (IsLibraryBlacklisted(GetLibraryNameWithoutPath(wideStringConverter.to_bytes(lpLibFileName))))
	{
		SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
		return NULL;
	}
	return ((LoadLibraryExW_T)HookLoadLibraryExWReturnAddress)(lpLibFileName, hFile, dwFlags);
}

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
		HMODULE kernelBaseHandle = GetModuleHandleA("KERNELBASE.dll");
		if (kernelBaseHandle == NULL)
		{
			return 0;
		}
		uintptr_t addrLoadLibraryA = (uintptr_t)GetProcAddress(kernelBaseHandle, "LoadLibraryA");
		uintptr_t addrLoadLibraryW = (uintptr_t)GetProcAddress(kernelBaseHandle, "LoadLibraryW");
		uintptr_t addrLoadLibraryExA = (uintptr_t)GetProcAddress(kernelBaseHandle, "LoadLibraryExA");
		uintptr_t addrLoadLibraryExW = (uintptr_t)GetProcAddress(kernelBaseHandle, "LoadLibraryExW");

		logger.Log("LoadLibraryA: %p", addrLoadLibraryA);
		logger.Log("LoadLibraryW: %p", addrLoadLibraryW);
		logger.Log("LoadLibraryExA: %p", addrLoadLibraryExA);
		logger.Log("LoadLibraryExW: %p", addrLoadLibraryExW);
		MemoryUtils::PlaceHook(addrLoadLibraryA, (uintptr_t)&HookLoadLibraryA, &HookLoadLibraryAReturnAddress);
		MemoryUtils::PlaceHook(addrLoadLibraryW, (uintptr_t)&HookLoadLibraryW, &HookLoadLibraryWReturnAddress);
		MemoryUtils::PlaceHook(addrLoadLibraryExA, (uintptr_t)&HookLoadLibraryExA, &HookLoadLibraryExAReturnAddress);
		MemoryUtils::PlaceHook(addrLoadLibraryExW, (uintptr_t)&HookLoadLibraryExW, &HookLoadLibraryExWReturnAddress);
		CreateThread(0, 0, &HookThread, 0, 0, NULL);
	}

	return 1;
}