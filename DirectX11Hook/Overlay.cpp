#include "Overlay.h"
#include "Renderer.h"

using namespace DirectX;

Overlay::Overlay(Renderer* renderer, DebugConsole* console)
{
	textures = renderer->GetTextures();
	fonts = renderer->GetFonts();
	this->console = console;
	
	ReadConfig();
}

void Overlay::Draw()
{
	spriteBatch->Begin();

	if (DoBox(XMINT2(500, 500), XMINT2(100, 100), textures->GetTexture(0).Get(), nullptr))
	{

	}

	//spriteBatch->Draw(textures->GetTexture(0).Get(), XMFLOAT2(100, 100));
	//const char* text = "Overlay drawing here!";
	//
	//console->PrintDebugMsg("Addr renderer: %p", (void*)renderer);
	//renderer->exampleFont->DrawString(spriteBatch.get(), text, XMFLOAT2(100, 100));

	spriteBatch->End();
}

bool Overlay::DoBox(DirectX::XMINT2 pos, DirectX::XMINT2 size, ID3D11ShaderResourceView* texture, bool* hover)
{
	RECT rect;
	rect.left = pos.x;
	rect.top = pos.y;
	rect.right = size.x;
	rect.bottom = size.y;
	spriteBatch->Draw(textures->GetTexture(0).Get(), rect);
	return false;
}

void Overlay::ReadValues()
{
}

void Overlay::ReadConfig()
{
}

void Overlay::LoadEverything()
{
	console->PrintDebugMsg("Loading textures...", nullptr, MsgType::STARTPROCESS);

	// Example textures
	textures->LoadTexture(".\\hook_textures\\texture.dds");
	textures->LoadTexture(".\\hook_textures\\texture2.dds");

	console->PrintDebugMsg("Loading fonts...", nullptr, MsgType::STARTPROCESS);

	// Default font
	fonts->LoadFont(".\\hook_fonts\\arial_22.spritefont");

	console->PrintDebugMsg("Loading complete, now rendering...", nullptr, MsgType::STARTPROCESS);
}

void Overlay::SetSpriteBatch(DirectX::SpriteBatch* spriteBatch)
{
	this->spriteBatch = std::shared_ptr<SpriteBatch>(spriteBatch);
}

// Example text
//Text test = Text("Hello there", 0.0f, 0.0f, 0, fonts->GetFont(0), renderer->GetWindowWidth(), renderer->GetWindowHeight());
//test.SetPos(test.GetPos().x - test.GetTextMidpointX(), test.GetPos().y - test.GetTextMidpointY());


//// Attempt to create vertex and index buffers for the given mesh
//void Core::AddMeshForDrawing(Mesh mesh)
//{
//	HRESULT VBResult = renderer.CreateBufferForMesh(mesh.GetVertexDesc(), mesh.GetVertexSubData(), mesh.GetVertexBuffer());
//	_com_error VBErr(VBResult);
//	console->PrintDebugMsg("CreateBuffer (VB) HRESULT: %s", (void*)VBErr.ErrorMessage());
//
//	if (FAILED(VBResult))
//	{
//		console->PrintDebugMsg("Failed to create vertex buffer for mesh", nullptr, MsgType::FAILED);
//		return;
//	}
//
//	HRESULT IBResult = renderer.CreateBufferForMesh(mesh.GetIndexDesc(), mesh.GetIndexSubData(), mesh.GetIndexBuffer());
//	_com_error IBErr(IBResult);
//	console->PrintDebugMsg("CreateBuffer (IB) HRESULT: %s", (void*)IBErr.ErrorMessage());
//
//	if (FAILED(IBResult))
//	{
//		console->PrintDebugMsg("Failed to create index buffer for mesh", nullptr, MsgType::FAILED);
//		return;
//	}
//
//	console->PrintDebugMsg("Successfully loaded mesh for rendering: %s", (void*)mesh.GetMeshClassName().c_str());
//	thingsToDraw.push_back(mesh);
//}
//
//void Core::AddTextForDrawing(Text text)
//{
//	textToDraw.push_back(text);
//}
