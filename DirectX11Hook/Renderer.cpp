#include "Renderer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

/*
* We load the shaders at compile time into a constant (shaderData) by converting
* the shader code file into a string. By doing this we don't need to bring the
* shader file around next to our .dll for it to get loaded, because it's embedded 
* instead. 
* 
* Found how to do this here: 
* https://stackoverflow.com/questions/20443560/how-to-practically-ship-glsl-shaders-with-your-c-software
*/
const char* shaderData = {
#include "Shaders.hlsl"
};



Renderer::Renderer(DebugConsole* console, bool drawExamples)
{
	this->console = console;
	this->drawExamples = drawExamples;
	textures = Textures(console);
	fonts = Fonts(console);
	overlay = Overlay(this, console);
}

bool Renderer::Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{

	if (firstInit)
	{
		console->PrintDebugMsg("Initializing renderer...", nullptr, MsgType::STARTPROCESS);
		HRESULT getDevice = swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);

		if (SUCCEEDED(getDevice))
		{
			device->GetImmediateContext(&context);
			spriteBatch = std::make_shared<SpriteBatch>(context.Get());
		}
		else
		{
			return false;
		}

	}
	else
	{
		console->PrintDebugMsg("Resizing buffers...", nullptr, MsgType::STARTPROCESS);
	}

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChain->GetDesc(&desc);

	RECT hwndRect;
	GetClientRect(desc.OutputWindow, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;
	console->PrintDebugMsg("Window width: %i", (void*)windowWidth, MsgType::PROGRESS);
	console->PrintDebugMsg("Window height: %i", (void*)windowHeight, MsgType::PROGRESS);

	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Width = windowWidth;
	viewport.Height = windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	ComPtr<ID3D11Texture2D> backbuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.GetAddressOf());
	device->CreateRenderTargetView(backbuffer.Get(), nullptr, &mainRenderTargetView);
	backbuffer.ReleaseAndGetAddressOf();

	if (firstInit)
	{
		CreatePipeline();
		CreateExampleTriangle();
		CreateExampleFont();

		textures.SetDevice(device.Get());
		fonts.SetDevice(device.Get());
		overlay.SetSpriteBatch(spriteBatch.get());

		console->PrintDebugMsg("Successfully initialized the renderer", nullptr, MsgType::COMPLETE);

		overlay.LoadEverything();
	}

	initialized = true;
	firstInit = false;
	return true;
}

void Renderer::Render(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	if (!initialized) return;

	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), nullptr);
	context->RSSetViewports(1, &viewport);

	overlay.Draw();

	// Spritebatch modifies certain rendering settings, so we draw them last
	//spriteBatch->Begin();
	//for (int i = 0; i < textToDraw.size(); i++)
	//{
	//	Text text = textToDraw.at(i);
	//	SpriteFont* font = fonts->GetFont(textToDraw.at(i).GetFontIndex());
	//	if(font != nullptr) 
	//		font->DrawString(spriteBatch.get(), text.GetText(), XMFLOAT2(text.GetPosPixels(windowWidth, windowHeight).x, text.GetPosPixels(windowWidth, windowHeight).y));
	//}
	//spriteBatch->End();

	//if (drawExamples)
	//{
	//	DrawExampleTriangle(); // Example triangle for render testing
	//	DrawExampleText(); // Same but with text
	//}

	return;
}

void Renderer::CreatePipeline()
{
	ComPtr<ID3DBlob> vertexShaderBlob = LoadShader(shaderData, "vs_5_0", "VS").Get();
	ComPtr<ID3DBlob> pixelShaderTexturesBlob = LoadShader(shaderData, "ps_5_0", "PSTex").Get();
	ComPtr<ID3DBlob> pixelShaderBlob = LoadShader(shaderData, "ps_5_0", "PS").Get();

	device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());

	device->CreatePixelShader(pixelShaderTexturesBlob->GetBufferPointer(),
		pixelShaderTexturesBlob->GetBufferSize(), nullptr, pixelShaderTextures.GetAddressOf());

	device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[3] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	device->CreateInputLayout(inputLayoutDesc, ARRAYSIZE(inputLayoutDesc), vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), inputLayout.GetAddressOf());

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, &samplerState);
}

ComPtr<ID3DBlob> Renderer::LoadShader(const char* shader, std::string targetShaderVersion, std::string shaderEntry)
{
	console->PrintDebugMsg("Loading shader: %s", (void*)shaderEntry.c_str(), MsgType::PROGRESS);
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ComPtr<ID3DBlob> shaderBlob;

	D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, shaderEntry.c_str(), targetShaderVersion.c_str(), D3DCOMPILE_ENABLE_STRICTNESS, 0, shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
	{
		char error[256]{ 0 };
		memcpy(error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
		console->PrintDebugMsg("Shader error: %s", (void*)error, MsgType::FAILED);
		return nullptr;
	}

	console->PrintDebugMsg("Shader loaded", nullptr, MsgType::PROGRESS);
	return shaderBlob;
}

HRESULT Renderer::CreateBufferForMesh(D3D11_BUFFER_DESC desc, D3D11_SUBRESOURCE_DATA data, ID3D11Buffer** buffer)
{
	HRESULT hr = device->CreateBuffer(&desc, &data, buffer);
	return hr;
}

void Renderer::CreateExampleTriangle()
{
	Vertex vertices[] =
	{
		{ XMFLOAT3(0.5f, 0.1f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.5f, 0.0f) },
		{ XMFLOAT3(0.6f, -0.1f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.5f) },
		{ XMFLOAT3(0.4f, -0.1f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.5f) }
	};

	D3D11_BUFFER_DESC vbDesc = { 0 };
	ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
	vbDesc.ByteWidth = sizeof(Vertex) * ARRAYSIZE(vertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA subData = { vertices, 0, 0 };

	device->CreateBuffer(&vbDesc, &subData, vertexBuffer.GetAddressOf());
}

void Renderer::CreateExampleFont()
{
	std::fstream file = std::fstream(".\\hook_fonts\\arial_22.spritefont");

	if (!file.fail())
	{
		file.close();
		exampleFont = std::make_shared<SpriteFont>(device.Get(), L".\\hook_fonts\\arial_22.spritefont");
	}

}

void Renderer::DrawExampleTriangle()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);

	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->Draw(3, 0);
}

void Renderer::DrawExampleText()
{
	if (spriteBatch == nullptr || exampleFont == nullptr) return;

	const char* text = "Hello, World!";
	const char* text2 = "This is a DirectX 11 hook";
	XMFLOAT2 stringSize, stringSize2;
	XMVECTOR textVector = exampleFont->MeasureString(text);
	XMVECTOR textVector2 = exampleFont->MeasureString(text2);
	XMStoreFloat2(&stringSize, textVector);
	XMStoreFloat2(&stringSize2, textVector2);

	spriteBatch->Begin();
	exampleFont->DrawString(spriteBatch.get(), text, XMFLOAT2((windowWidth / 2) - (stringSize.x / 2), (windowHeight * 0.335) - (stringSize.y / 2)));
	exampleFont->DrawString(spriteBatch.get(), text2, XMFLOAT2((windowWidth / 2) - (stringSize2.x / 2), (windowHeight * 0.665) - (stringSize2.y / 2)));
	spriteBatch->End();
}

void Renderer::OnPresent(IDXGISwapChain * swapChain, UINT syncInterval, UINT flags)
{

	if (!initialized)
	{
		if (!Init(swapChain, syncInterval, flags)) return;
	}

	Render(swapChain, syncInterval, flags);
}

void Renderer::OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	initialized = false;
	mainRenderTargetView.ReleaseAndGetAddressOf();
}

void Renderer::SetDrawExamples(bool doDraw)
{
	drawExamples = doDraw;
}

Textures* Renderer::GetTextures()
{
	return &textures;
}

Fonts* Renderer::GetFonts()
{
	return &fonts;
}

bool Renderer::IsInitialized()
{
	return initialized;
}

int Renderer::GetWindowWidth()
{
	return windowWidth;
}

int Renderer::GetWindowHeight()
{
	return windowHeight;
}

ID3D11Device* Renderer::GetDevice()
{
	return device.Get();
}


//context->VSSetShader(vertexShader.Get(), nullptr, 0);
//context->PSSetShader(pixelShaderTextures.Get(), nullptr, 0);

//context->PSSetSamplers(0, 1, samplerState.GetAddressOf());

//UINT stride = sizeof(Vertex);
//UINT offset = 0;

//for (int i = 0; i < thingsToDraw.size(); i++)
//{
//	Mesh mesh = thingsToDraw.at(i);
	//ComPtr<ID3D11ShaderResourceView> texture = textures->GetTexture(mesh.GetTextureIndex()).Get();

	//if (mesh.GetTextureIndex() == -1 || texture.Get() == nullptr)
	//{
		//context->PSSetShader(pixelShader.Get(), nullptr, 0);
	//}
	//else
	//{
	//	context->PSSetShaderResources(0, 1, texture.GetAddressOf());
	//}

	//context->IASetVertexBuffers(0, 1, mesh.GetVertexBuffer(), &stride, &offset);
	//context->IASetIndexBuffer(*mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
	//context->DrawIndexed(mesh.GetNumIndices(), 0, 0);
//}