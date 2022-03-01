#include <Windows.h>

#include "DirectXHook.h"
#include "Overlays/PauseEldenRing/PauseEldenRing.h"

// JmpToAddr() is a function written in assembly.
// We use this function to do primitive jumps to function addresses without any strings attached.
extern "C" int JmpToAddr();
FARPROC dxgiFunctions[21];
bool reshadeLoaded = false;

DWORD WINAPI MainThread(LPVOID lpParam)
{
	DirectXHook dxHook;
	static PauseEldenRing pauseEldenRing;
	dxHook.DrawExamples(false);
	dxHook.Hook();
	dxHook.HandleReshade(reshadeLoaded);
	dxHook.SetRenderCallback(&pauseEldenRing);
	return S_OK;
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	HMODULE dxgi = 0;

	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(module);

		dxgi = LoadLibrary("C:\\Windows\\System32\\dxgi.dll");
		if (!dxgi)
		{
			return false;
		}

		// Gives us some more control over ReShade...
		if (LoadLibrary("reshade.dll"))
		{
			reshadeLoaded = true;
		}

		// Find function addresses we need for forward exporting
		dxgiFunctions[0] = GetProcAddress(dxgi, "ApplyCompatResolutionQuirking");
		dxgiFunctions[1] = GetProcAddress(dxgi, "CompatString");
		dxgiFunctions[2] = GetProcAddress(dxgi, "CompatValue");
		dxgiFunctions[3] = GetProcAddress(dxgi, "CreateDXGIFactory");
		dxgiFunctions[4] = GetProcAddress(dxgi, "CreateDXGIFactory1");
		dxgiFunctions[5] = GetProcAddress(dxgi, "CreateDXGIFactory2");
		dxgiFunctions[6] = GetProcAddress(dxgi, "DXGID3D10CreateDevice");
		dxgiFunctions[7] = GetProcAddress(dxgi, "DXGID3D10CreateLayeredDevice");
		dxgiFunctions[8] = GetProcAddress(dxgi, "DXGID3D10ETWRundown");
		dxgiFunctions[9] = GetProcAddress(dxgi, "DXGID3D10GetLayeredDeviceSize");
		dxgiFunctions[10] = GetProcAddress(dxgi, "DXGID3D10RegisterLayers");
		dxgiFunctions[11] = GetProcAddress(dxgi, "DXGIDeclareAdapterRemovalSupport");
		dxgiFunctions[12] = GetProcAddress(dxgi, "DXGIDumpJournal");
		dxgiFunctions[13] = GetProcAddress(dxgi, "DXGIGetDebugInterface1");
		dxgiFunctions[14] = GetProcAddress(dxgi, "DXGIReportAdapterConfiguration");
		dxgiFunctions[15] = GetProcAddress(dxgi, "DXGIRevertToSxS");
		dxgiFunctions[16] = GetProcAddress(dxgi, "PIXBeginCapture");
		dxgiFunctions[17] = GetProcAddress(dxgi, "PIXEndCapture");
		dxgiFunctions[18] = GetProcAddress(dxgi, "PIXGetCaptureState");
		dxgiFunctions[19] = GetProcAddress(dxgi, "SetAppCompatStringPointer");
		dxgiFunctions[20] = GetProcAddress(dxgi, "UpdateHMDEmulationStatus");

		// Start hooking on a new thread inside the current process
		CreateThread(0, 0x1000, &MainThread, 0, 0, NULL);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		FreeLibrary(dxgi);
		return 1;
	}

	return 1;
}

/* 
* These are the functions our .dll exports for the application to use.
* We jump to the real .dll's exported functions to perform these for us (because we don't actually want
* to implement these functions).
* I used a third-party tool to automatically create a template proxy .dll,
* https://www.codeproject.com/Articles/1179147/ProxiFy-Automatic-Proxy-DLL-Generation
* By using this you don't have to manually write all the exports or the module definition file (.def)
*/
extern "C"
{
	FARPROC functionAddress = NULL;

	void PROXY_ApplyCompatResolutionQuirking() {
		functionAddress = dxgiFunctions[0];
		JmpToAddr();
	}
	void PROXY_CompatString() {
		functionAddress = dxgiFunctions[1];
		JmpToAddr();
	}
	void PROXY_CompatValue() {
		functionAddress = dxgiFunctions[2];
		JmpToAddr();
	}
	void PROXY_CreateDXGIFactory() {
		functionAddress = dxgiFunctions[3];
		JmpToAddr();
	}
	void PROXY_CreateDXGIFactory1() {
		functionAddress = dxgiFunctions[4];
		JmpToAddr();
	}
	void PROXY_CreateDXGIFactory2() {
		functionAddress = dxgiFunctions[5];
		JmpToAddr();
	}
	void PROXY_DXGID3D10CreateDevice() {
		functionAddress = dxgiFunctions[6];
		JmpToAddr();
	}
	void PROXY_DXGID3D10CreateLayeredDevice() {
		functionAddress = dxgiFunctions[7];
		JmpToAddr();
	}
	void PROXY_DXGID3D10ETWRundown() {
		functionAddress = dxgiFunctions[8];
		JmpToAddr();
	}
	void PROXY_DXGID3D10GetLayeredDeviceSize() {
		functionAddress = dxgiFunctions[9];
		JmpToAddr();
	}
	void PROXY_DXGID3D10RegisterLayers() {
		functionAddress = dxgiFunctions[10];
		JmpToAddr();
	}
	void PROXY_DXGIDeclareAdapterRemovalSupport() {
		functionAddress = dxgiFunctions[11];
		JmpToAddr();
	}
	void PROXY_DXGIDumpJournal() {
		functionAddress = dxgiFunctions[12];
		JmpToAddr();
	}
	void PROXY_DXGIGetDebugInterface1() {
		functionAddress = dxgiFunctions[13];
		JmpToAddr();
	}
	void PROXY_DXGIReportAdapterConfiguration() {
		functionAddress = dxgiFunctions[14];
		JmpToAddr();
	}
	void PROXY_DXGIRevertToSxS() {
		functionAddress = dxgiFunctions[15];
		JmpToAddr();
	}
	void PROXY_PIXBeginCapture() {
		functionAddress = dxgiFunctions[16];
		JmpToAddr();
	}
	void PROXY_PIXEndCapture() {
		functionAddress = dxgiFunctions[17];
		JmpToAddr();
	}
	void PROXY_PIXGetCaptureState() {
		functionAddress = dxgiFunctions[18];
		JmpToAddr();
	}
	void PROXY_SetAppCompatStringPointer() {
		functionAddress = dxgiFunctions[19];
		JmpToAddr();
	}
	void PROXY_UpdateHMDEmulationStatus() {
		functionAddress = dxgiFunctions[20];
		JmpToAddr();
	}
}
