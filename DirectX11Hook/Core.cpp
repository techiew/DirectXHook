#include "Core.h"

// Variables shared with assembly code
extern "C" int JmpToHookAndJmpBack();
extern "C" MEMADDR presentAddr = NULL;
extern "C" MEMADDR jmpBackAddr = NULL;

bool first = true;
Core* coreRef = nullptr;

// The new code that the hook redirects towards
HRESULT __stdcall Present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{

	if (first)
	{
		DebugConsole* consoleRef = &coreRef->console;
		consoleRef->PrintDebugMsg("Hello from the hooked function", nullptr, MsgType::PROGRESS);
		consoleRef->PrintDebugMsg("Swap chain address: %p", (void*)swapChain, MsgType::COMPLETE);
		first = false;
	}

	coreRef->Render(swapChain, syncInterval, flags);

	return ((PresentFunction)coreRef->newPresentReturn)(swapChain, syncInterval, flags);
}




void Core::Init(HMODULE originalDll)
{
	// Settings
	drawExamples = true; // Whether or not to draw the example rainbow triangle and text
	showDebugConsole = true;

	this->originalDll = originalDll;
	coreRef = this;
	console = DebugConsole("DirectX11Hook", showDebugConsole);
	textures = Textures(&console);
	fonts = Fonts(&console);
	renderer = Renderer(&console, &textures, &fonts);

	console.PrintDebugMsg("Initializing hook...", nullptr, MsgType::STARTPROCESS);

	char currentPath[260];
	console.PrintDebugMsg("Current working directory: %s", _getcwd(currentPath, sizeof(currentPath)), MsgType::PROGRESS);

	originalDllBaseAddress = (MEMADDR)originalDll;

#ifdef is64bit
	originalPresentFunctionOffset = 0x5070;
#else
	originalPresentFunctionOffset = 0x10230;
#endif

	originalPresentFunction = (PresentFunction)(originalDllBaseAddress + (MEMADDR)originalPresentFunctionOffset);

	console.PrintDebugMsg("dxgi.dll base address: %p", (void*)originalDllBaseAddress, MsgType::PROGRESS);
	console.PrintDebugMsg("Present offset: %p", (void*)originalPresentFunctionOffset, MsgType::PROGRESS);
	console.PrintDebugMsg("Present address: %p", (void*)originalPresentFunction, MsgType::PROGRESS);

#ifdef is64bit
	Hook((MEMADDR)originalPresentFunction, (MEMADDR)JmpToHookAndJmpBack, 14);
#else
	Hook((MEMADDR)originalPresentFunction, (MEMADDR)JmpToHookAndJmpBack, 5);
#endif

	console.PrintDebugMsg("Present function hooked successfully", nullptr, MsgType::PROGRESS);

	Update();
}

void Core::Hook(MEMADDR originalFunction, MEMADDR newFunction, int length)
{
	DWORD oldProtection;
	DWORD oldProtection2;

	VirtualProtect((void*)originalFunction, length, PAGE_EXECUTE_READWRITE, &oldProtection);

	// Copy original bytes to a buffer
	originalInstructionsBuffer = std::make_unique<std::string>(length, '*');
	memcpy(originalInstructionsBuffer.get(), (void*)originalFunction, length);

	// Print original bytes to console for debugging
	console.PrintDebugMsg("Original instructions buffer: ", nullptr, MsgType::PROGRESS);
	for (int i = 0; i < originalInstructionsBuffer.get()->length(); i++)
	{
		console.PrintSingleChar(originalInstructionsBuffer.get()->c_str()[i], true);
	}
	console.PrintSingleChar('\n', false);

	VirtualProtect((void*)newFunction, length, PAGE_EXECUTE_READWRITE, &oldProtection2);

	// Copy original instructions to the end of the new function, that get executed after we have done our rendering
	memcpy((MEMADDR*)newFunction + 2, originalInstructionsBuffer.get(), length);

	VirtualProtect((void*)newFunction, length, oldProtection2, &oldProtection2);

	memset((void*)originalFunction, 0x90, length);

	/*
	* We will now place an absolute 64-bit jump (FF 25 00000000) to our new detour function
	* Bytes are flipped around by default (because of 'endianness'), so we use _byteswap to flip back for easier reading
	* We also fill the _byteswap function with a full 8 bytes so it doesn't return garbage
	*
	* An absolute 64-bit jump instruction uses only 6 bytes on its own,
	* but then reads an 8-byte address from the subsequent memory address
	*
	* Else if it's 32-bit, do a normal jump (E9)
	*/

#ifdef is64bit
	*((MEMADDR*)originalFunction) = _byteswap_uint64(0xFF25000000000000);
	*((MEMADDR*)(originalFunction + 6)) = newFunction;
#else
	MEMADDR relativeAddress = ((MEMADDR)newFunction - (MEMADDR)originalFunction) - 5;
	*((MEMADDR*)originalFunction) = _byteswap_ulong(0xE9000000);
	*((MEMADDR*)(originalFunction + 1)) = relativeAddress;
#endif

	VirtualProtect((void*)originalFunction, length, oldProtection, &oldProtection);

	// Setting the pointer to our new present function, the assembly code uses it to jump there
	presentAddr = (MEMADDR)Present;

	// Setting the memory address for the return from our new present function back to the assembly code
	newPresentReturn = (MEMADDR)((MEMADDR*)newFunction + 2);

	console.PrintDebugMsg("New Present return: %p", (void*)newPresentReturn, MsgType::PROGRESS);

	// Setting the memory address for a jump from our assembly code back to the original present function
	jmpBackAddr = originalFunction + length;

	return;
}

void Core::Update()
{
	/* 
	* Update things here if needed.
	* This does not block as the rendering is done by another thread from
	* the main application through the hooked function
	*/
	//while (true)
	//{
	//
	//}
}

void Core::Render(IDXGISwapChain *swapChain, UINT syncInterval, UINT flags)
{

	if (!renderer.IsInitialized())
	{
		if(!renderer.Init(swapChain, syncInterval, flags)) return;
	}

	if (!texturesLoaded)
	{
		console.PrintDebugMsg("Loading textures...", nullptr, MsgType::STARTPROCESS);

		// Example textures
		textures.SetDevice(renderer.GetDevice());
		textures.LoadTexture(".\\hook_textures\\texture.dds");
		textures.LoadTexture(".\\hook_textures\\texture2.dds");

		texturesLoaded = true;
		console.PrintDebugMsg("All textures loaded", nullptr, MsgType::COMPLETE);
	}

	if (!meshesCreated)
	{
		console.PrintDebugMsg("Loading meshes...", nullptr, MsgType::STARTPROCESS);

		// Textured example boxes
		//AddMeshForDrawing(TexturedBox(-0.3f, 0.0f, 0.4f, 0.4f, 0));
		//AddMeshForDrawing(TexturedBox(0.3f, 0.0f, 0.4f, 0.4f, 1));

		meshesCreated = true;
		console.PrintDebugMsg("All meshes loaded", nullptr, MsgType::COMPLETE);
	}

	if (!fontsLoaded)
	{
		console.PrintDebugMsg("Loading fonts...", nullptr, MsgType::STARTPROCESS);
		
		// Default font
		fonts.SetDevice(renderer.GetDevice());
		fonts.LoadFont(".\\hook_fonts\\arial_22.spritefont");

		fontsLoaded = true;
		console.PrintDebugMsg("All fonts loaded", nullptr, MsgType::COMPLETE);
	}

	if (!textCreated)
	{
		console.PrintDebugMsg("Loading text...", nullptr, MsgType::STARTPROCESS);

		// Example text
		Text test = Text("Hello there", 0.0f, 0.0f, 0, fonts.GetFont(0), renderer.GetWindowWidth(), renderer.GetWindowHeight());
		test.SetPos(test.GetPos().x - test.GetTextMidpointX(), test.GetPos().y - test.GetTextMidpointY());
		//AddTextForDrawing(test);

		textCreated = true;
		console.PrintDebugMsg("All text loaded", nullptr, MsgType::COMPLETE);
	}

	if (renderer.IsFirstRender())
	{
		console.PrintDebugMsg("Pre-render stage complete, now rendering...", nullptr, MsgType::STARTPROCESS);
		renderer.SetFirstRender(false);
	}

	renderer.Render(swapChain, syncInterval, flags, thingsToDraw, textToDraw, drawExamples);
}

// Attempt to create vertex and index buffers for the given mesh
void Core::AddMeshForDrawing(Mesh mesh)
{
	HRESULT VBResult = renderer.CreateBufferForMesh(mesh.GetVertexDesc(), mesh.GetVertexSubData(), mesh.GetVertexBuffer());
	_com_error VBErr(VBResult);
	console.PrintDebugMsg("CreateBuffer (VB) HRESULT: %s", (void*)VBErr.ErrorMessage(), MsgType::PROGRESS);

	if (FAILED(VBResult))
	{
		console.PrintDebugMsg("Failed to create vertex buffer for mesh", nullptr, MsgType::FAILED);
		return;
	}

	HRESULT IBResult = renderer.CreateBufferForMesh(mesh.GetIndexDesc(), mesh.GetIndexSubData(), mesh.GetIndexBuffer());
	_com_error IBErr(IBResult);
	console.PrintDebugMsg("CreateBuffer (IB) HRESULT: %s", (void*)IBErr.ErrorMessage(), MsgType::PROGRESS);

	if (FAILED(IBResult))
	{
		console.PrintDebugMsg("Failed to create index buffer for mesh", nullptr, MsgType::FAILED);
		return;
	}

	console.PrintDebugMsg("Successfully loaded mesh for rendering: %s", (void*)mesh.GetMeshClassName().c_str(), MsgType::PROGRESS);
	thingsToDraw.push_back(mesh);
}

void Core::AddTextForDrawing(Text text)
{
	textToDraw.push_back(text);
}

Core::~Core()
{
	FreeConsole();
}
