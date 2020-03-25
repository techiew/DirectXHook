#include "Core.h"

Core* coreRef = nullptr;
bool first = true;
bool safeguard = false;

/* 
* The real Present will get hooked and then detour to this function.
* Present is part of the final rendering stage in DirectX.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
*/
HRESULT __stdcall Present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{

	if (first)
	{
		coreRef->console.PrintDebugMsg("Hello from the hooked Present function", nullptr);
		first = false;
	}

	coreRef->OnPresent(swapChain, syncInterval, flags);

	return ((PresentFunction)coreRef->newPresentReturn)(swapChain, syncInterval, flags);
}

/*
* The real ResizeBuffers will get hooked and then detour to this function.
* ResizeBuffers usually gets called when the window resizes (not all games call it).
* We need to hook this so we can release our reference to the render target when it's called.
* If we don't do this then the game will most likely crash.
* https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
*/
HRESULT __stdcall ResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	coreRef->console.PrintDebugMsg("ResizeBuffers was called", nullptr);

	coreRef->OnResizeBuffers(bufferCount, width, height, newFormat, swapChainFlags);

	return ((ResizeBuffersFunction)coreRef->newResizeBuffersReturn)(bufferCount, width, height, newFormat, swapChainFlags);
}





// Initialize and start hooking
void Core::Init(HMODULE originalDll)
{
	// Settings
	drawExamples = true; // Whether or not to draw the example rainbow triangle and text
	showDebugConsole = true;

	this->originalDll = originalDll;
	coreRef = this;
	console = DebugConsole("DirectX11Hook", showDebugConsole);
	renderer = Renderer(&console, drawExamples);
	scanner = SigScanner();

	console.PrintDebugMsg("Initializing hook...", nullptr, MsgType::STARTPROCESS);

	// Print the current working directory
	char currentPath[260];
	console.PrintDebugMsg("Current working directory: %s", _getcwd(currentPath, sizeof(currentPath)));

	targetDllBaseAddress = (MEMADDR)originalDll;

	// Check if the Steam overlay is active
#ifdef _WIN64
	HMODULE gorModule = GetModuleHandle("gameoverlayrenderer64.dll");
	steamDllName = "gameoverlayrenderer64.dll";
#else
	HMODULE gorModule = GetModuleHandle("gameoverlayrenderer.dll");
	steamDllName = "gameoverlayrenderer.dll";
#endif

	MODULEINFO gorInfo;

	if (gorModule != 0)
	{
		console.PrintDebugMsg("Steam overlay is active, changing target!");
		GetModuleInformation(GetCurrentProcess(), gorModule, &gorInfo, sizeof(MODULEINFO));
		targetDllBaseAddress = (MEMADDR)gorInfo.lpBaseOfDll;
		steamOverlayActive = true;
	}

	// Find the function addresses that we need (also gets the offsets from the .dll base, for debugging purposes)
	targetResizeBuffersFunction = FindResizeBuffersAddress(steamOverlayActive);
	targetPresentFunction = FindPresentAddress(steamOverlayActive);
	targetPresentOffset = (MEMADDR)targetPresentFunction - targetDllBaseAddress;
	targetResizeBuffersOffset = (MEMADDR)targetResizeBuffersFunction - targetDllBaseAddress;

	if (targetResizeBuffersFunction == 0 || targetPresentFunction == 0) return;

	if (steamOverlayActive)
	{
		console.PrintDebugMsg(steamDllName + (std::string)" base address: %p", (void*)targetDllBaseAddress);
	}
	else
	{
		console.PrintDebugMsg("dxgi.dll base address: %p", (void*)targetDllBaseAddress);
	}

	console.PrintDebugMsg("ResizeBuffers offset: %p", (void*)targetResizeBuffersOffset);
	console.PrintDebugMsg("ResizeBuffers address: %p", (void*)targetResizeBuffersFunction);
	console.PrintDebugMsg("Present offset: %p", (void*)targetPresentOffset);
	console.PrintDebugMsg("Present address: %p", (void*)targetPresentFunction);

	Hook(targetResizeBuffersFunction, &ResizeBuffers, &newResizeBuffersReturn, resizeBuffersHookSize);
	Hook(targetPresentFunction, &Present, &newPresentReturn, presentHookSize);

	console.PrintDebugMsg("Functions hooked successfully", nullptr, MsgType::COMPLETE);

	Update();
}

// Allocate some memory for our hooks
void Core::AllocateMemory(void** storePointer, int size)
{
	*storePointer = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (storePointer == nullptr)
	{
		console.PrintDebugMsg("Failed to allocate memory of %i bytes", (void*)size, MsgType::FAILED);
	}
	else
	{
		memset(*storePointer, 0x90, size);
		console.PrintDebugMsg("Allocated space for a hook at %p", *storePointer);
	}

}

// Signature scan for ResizeBuffers
ResizeBuffersFunction* Core::FindResizeBuffersAddress(bool hookSteamOverlay)
{
	console.PrintDebugMsg("Sigscanning for address of ResizeBuffers...");
	const char* bytes;
	const char* mask;
	int offset = 0;
	void* resizeBuffersAddress;

	if (hookSteamOverlay)
	{
#ifdef _WIN64
		// 64-bit with steam overlay
		resizeBuffersHookSize = 15;
		bytes = "\x41\x8B\xE9\x48\x8D\x0D\x00\x00\x00\x00\x45\x8B\xF0\xE8";
		mask = "xxxxxx????xxxx";
		offset = -33;
#else
		// 32-bit with steam overlay
		resizeBuffersHookSize = 7;
		bytes = "\x55\x8B\xEC\x53\x8B\x5D\x00\xB9\x00\x00\x00\x00\x56\x57\x53\xE8\x00\x00\x00\x00\x53";
		mask = "xxxxxx?x????xxxx????x";
		offset = 0;
#endif
		resizeBuffersAddress = (char*)scanner.FindPattern(steamDllName, bytes, mask) + offset;
	}
	else
	{
#ifdef _WIN64
		// 64-bit without steam overlay
		resizeBuffersHookSize = 16;
		bytes = "\x8B\x75\x7F\x89\x74\x24\x30\x44\x8B\x75\x77\x44\x89\x74\x24\x28";
		mask = "xxxxxxxxxxxxxxxx";
		offset = -55;
#else
		// 32-bit without steam overlay
		resizeBuffersHookSize = 5;
		bytes = "\x8B\x4D\x1C\x8B\x5D\x08\x89\x85\x68\xFF\xFF\xFF";
		mask = "xxxxxxxxxxxx";
		offset = -20;
#endif
		resizeBuffersAddress = (char*)scanner.FindPattern(originalDll, bytes, mask) + offset;
	}

	if (resizeBuffersAddress == 0)
	{
		console.PrintDebugMsg("ResizeBuffers address was not found.", nullptr, MsgType::FAILED);
	}

	return (ResizeBuffersFunction*)resizeBuffersAddress;
}

// Signature scan for Present
PresentFunction* Core::FindPresentAddress(bool hookSteamOverlay)
{
	console.PrintDebugMsg("Sigscanning for address of Present...");
	const char* bytes;
	const char* mask;
	int offset = 0;
	void* presentAddress;

	if (hookSteamOverlay)
	{
#ifdef _WIN64
		// 64-bit with steam overlay
		presentHookSize = 16;
		bytes = "\x48\x89\x00\x24\x18\x48\x89\x00\x24\x20\x41\x56\x48\x83\x00\x20\x41\x8B\xE8\x8B\xF2\x4C\x8B\xF1";
		mask = "xx?xxxx?xxxxxx?xxxxxxxxx";
		offset = 0;
#else
		// 32-bit with steam overlay
		presentHookSize = 7;
		bytes = "\x55\x8B\xEC\x53\x8B\x5D\x00\xF6\xC3\x01\x74\x00\x53";
		mask = "xxxxxx?xxxx?x";
		offset = 0;
#endif
		presentAddress = (char*)scanner.FindPattern(steamDllName, bytes, mask) + offset;
	}
	else
	{
#ifdef _WIN64
		// 64-bit without steam overlay
		presentHookSize = 14;
		bytes = "\x48\x89\x00\x00\xC6\x44\x24\x00\x00\x0F\x85\x00\x00\x00\x00\xF6\x05";
		mask = "xx??xxx??xx????xx";
		offset = -67;
#else
		// 32-bit without steam overlay
		presentHookSize = 5;
		bytes = "\x8D\x4C\x24\x0C\x57\xFF\x75\x10\xFF\x75\x0C\x56";
		mask = "xxxxxxxxxxxx";
		offset = -26;
#endif
		presentAddress = (char*)scanner.FindPattern(originalDll, bytes, mask) + offset;
	}

	if (presentAddress == 0)
	{
		console.PrintDebugMsg("Present address was not found.", nullptr, MsgType::FAILED);
	}

	return (PresentFunction*)presentAddress;
}

// The actual hooking, places assembly jumps between hookFrom and hookTo
void Core::Hook(void* hookFrom, void* hookTo, void* returnAddress, int length)
{
	console.PrintDebugMsg("Hooking from %p", hookFrom);
	console.PrintDebugMsg("To %p", hookTo);

	DWORD oldProtection;
	VirtualProtect(hookFrom, length, PAGE_EXECUTE_READWRITE, &oldProtection);

	// Copy any to-be-overwritten bytes to a buffer
	std::vector<unsigned char> bytesBuffer = std::vector<unsigned char>(length, '*');
	memcpy(&bytesBuffer[0], hookFrom, length);

	// Print overwritten bytes to console for debugging purposes
	console.PrintDebugMsg("Original bytes: ");
	for (int i = 0; i < bytesBuffer.size(); i++)
	{
		console.PrintHex(bytesBuffer.at(i));
	}
	console.NewLine();
	console.PrintDebugMsg("Size: %i", (void*)bytesBuffer.size());

	/*
	* We will now place an absolute 64-bit jump (0xFF25000000000000) which jumps to our assembly code.
	* The assembly code contains the instructions that will be overwritten after we place said jump.
	* It also handles jumping to the new function that we are detouring to and then handles jumping
	* back to the original code. The space for this assembly code is allocated manually beforehand
	* and the assembly instructions themselves are created by writing bytes directly into memory.
	*
	* _byteswap is only used to make the hex more readable, normally the bytes would have to be
	* inverted when we write them in. We have to fill the _byteswap function with a full 4 or 8 bytes, 
	* otherwise it returns garbage.
	*
	* An absolute 64-bit jump instruction (0xFF25000000000000) uses only 6 bytes on its own in memory,
	* but when it's called it will read an 8-byte address starting from the following memory address.
	*
	* If we are building for 32-bit, we do a normal, relative jump (0xE9000000)
	*
	* Resource for assembly jmps:
	* https://www.felixcloutier.com/x86/jmp
	*/

	MEMADDR hookMemory = 0;

#ifdef _WIN64
	// Allocates jmp size + length + return jmp size
	AllocateMemory((void**)&hookMemory, 14 + length + 14);

	*((MEMADDR*)hookMemory) = _byteswap_uint64(0xFF25000000000000);
	*((MEMADDR*)(hookMemory + 6)) = (MEMADDR)hookTo;

	memcpy((MEMADDR*)(hookMemory + 14), &bytesBuffer[0], length);

	*(MEMADDR*)returnAddress = hookMemory + 14;

	*((MEMADDR*)(hookMemory + 14 + length)) = _byteswap_uint64(0xFF25000000000000);
	*((MEMADDR*)(hookMemory + 14 + length + 6)) = (MEMADDR)hookFrom + length;

	memset(hookFrom, 0x90, length);

	*((MEMADDR*)hookFrom) = _byteswap_uint64(0xFF25000000000000);
	*((MEMADDR*)((char*)hookFrom + 6)) = hookMemory;

#else
	// Allocates jmp size + length + return jmp size
	AllocateMemory((void**)&hookMemory, 5 + length + 5);

	MEMADDR relativeAddress;

	relativeAddress = (MEMADDR)hookTo - hookMemory - 5;

	*((MEMADDR*)hookMemory) = _byteswap_ulong(0xE9000000);
	*((MEMADDR*)(hookMemory + 1)) = relativeAddress;

	memcpy((MEMADDR*)(hookMemory + 5), &bytesBuffer[0], length);

	*(MEMADDR*)returnAddress = hookMemory + 5;

	relativeAddress = ((MEMADDR)hookFrom + length) - (hookMemory + 5 + length) - 5;

	*((MEMADDR*)(hookMemory + 5 + length)) = _byteswap_ulong(0xE9000000);
	*((MEMADDR*)(hookMemory + 5 + length + 1)) = relativeAddress;

	memset(hookFrom, 0x90, length);

	relativeAddress = hookMemory - (MEMADDR)hookFrom - 5;

	*((MEMADDR*)hookFrom) = _byteswap_ulong(0xE9000000);
	*((MEMADDR*)((char*)hookFrom + 1)) = relativeAddress;

#endif

	console.PrintDebugMsg("Return address: %p", (void*)*(MEMADDR*)returnAddress, MsgType::COMPLETE);

	allocatedMemory.push_back((void*)hookMemory);

	VirtualProtect(hookFrom, length, oldProtection, &oldProtection);

	return;
}

/*
* Do things here if needed.
* This does not block because the rest of the code should run on another
* thread belonging to the main application that is running the hooked functions.
* I've mostly used this function for delayed hooking.
*/
void Core::Update()
{

	//while (true)
	//{
	//
	//	if (GetAsyncKeyState(VK_F4) && safeguard != true) // F4 button
	//	{
	//		Hook(targetResizeBuffersFunction, &ResizeBuffers, &newResizeBuffersReturn, resizeBuffersHookSize);
	//		Hook(targetPresentFunction, &Present, &newPresentReturn, presentHookSize);
	//		safeguard = true;
	//	}
	//	
	//	Sleep(50); // Let the CPU relax a little
	//}

}

// Event for when Present runs
void Core::OnPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	renderer.OnPresent(swapChain, syncInterval, flags);
}

// Event for when ResizeBuffers runs
void Core::OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	renderer.OnResizeBuffers(bufferCount, width, height, newFormat, swapChainFlags);
}

Core::~Core()
{

	for (int i = 0; i < allocatedMemory.size(); i++)
	{

		if (allocatedMemory.at(i) != nullptr)
		{
			VirtualFree(allocatedMemory.at(i), 0, MEM_RELEASE);
		}

	}

	FreeConsole();
}
