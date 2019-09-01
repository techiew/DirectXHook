#include "Renderer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

/*
* Load the shaders at compile time into a constant by converting
* the shader code into a string. By doing this we don't have to
* place the shaders next to our .dll (or embed in some other way).
* And we get to keep the nice HLSL formatting/highlighting in Visual Studio.
* 
* Found how to do this here: 
* https://stackoverflow.com/questions/20443560/how-to-practically-ship-glsl-shaders-with-your-c-software
*/
const char* shaderData = {
#include "Shaders.hlsl"
};



Renderer::Renderer(DebugConsole* console, Textures* textures, Fonts* fonts)
{
	this->console = console;
	this->textures = textures;
	this->fonts = fonts;
}

bool Renderer::Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	console->PrintDebugMsg("Initializing renderer...", nullptr, MsgType::STARTPROCESS);

	HRESULT getDevice = swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);

	if (SUCCEEDED(getDevice))
	{
		device->GetImmediateContext(&context);
	}
	else
	{
		console->PrintDebugMsg("Failed to initialize renderer", nullptr, MsgType::FAILED);
		return false;
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

	ComPtr<ID3D11Texture2D> backbuffer;

	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.GetAddressOf());
	device->CreateRenderTargetView(backbuffer.Get(), nullptr, &mainRenderTargetView);
	console->PrintDebugMsg("Backbuffer address: %p", backbuffer.Get(), MsgType::PROGRESS);

	// Sprite batch used for drawing text
	spriteBatch = std::make_shared<SpriteBatch>(context.Get());

	CreatePipeline();
	CreateExampleTriangle();
	CreateExampleFont();

	console->PrintDebugMsg("Successfully initialized the renderer", nullptr, MsgType::COMPLETE);
	initialized = true;
	return true;
}

bool Renderer::Render(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags, std::vector<Mesh> thingsToDraw, std::vector<Text> textToDraw, bool drawExamples)
{
	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), nullptr);
	context->RSSetViewports(1, &viewport);

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShaderTextures.Get(), nullptr, 0);

	context->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	for (int i = 0; i < thingsToDraw.size(); i++)
	{
		Mesh mesh = thingsToDraw.at(i);
		ComPtr<ID3D11ShaderResourceView> texture = textures->GetTexture(mesh.GetTextureIndex()).Get();

		if (mesh.GetTextureIndex() == -1 || texture.Get() == nullptr)
		{
			context->PSSetShader(pixelShader.Get(), nullptr, 0);
		}
		else
		{
			context->PSSetShaderResources(0, 1, texture.GetAddressOf());
		}

		context->IASetVertexBuffers(0, 1, mesh.GetVertexBuffer(), &stride, &offset);
		context->IASetIndexBuffer(*mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(mesh.GetNumIndices(), 0, 0);
	}

	if (drawExamples)
	{
		DrawExampleTriangle(); // Example triangle for render testing
		DrawExampleText(); // Same but with text
	}

	// Spritebatch modifies certain rendering settings, so we draw them last
	spriteBatch->Begin();
	for (int i = 0; i < textToDraw.size(); i++)
	{
		Text text = textToDraw.at(i);
		SpriteFont* font = fonts->GetFont(textToDraw.at(i).GetFontIndex());
		if(font != nullptr) 
			font->DrawString(spriteBatch.get(), text.GetText(), XMFLOAT2(text.GetPosPixels(windowWidth, windowHeight).x, text.GetPosPixels(windowWidth, windowHeight).y));
	}
	spriteBatch->End();

	return true;
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

	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Width = windowWidth;
	viewport.Height = windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

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
		{ XMFLOAT3(0.0f, 0.3f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.5f, 0.0f) },
		{ XMFLOAT3(0.2f, -0.3f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.5f) },
		{ XMFLOAT3(-0.2f, -0.3f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.5f) }
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
	// Create sprite batch for text, also make an example font
	// We need to check if the font exists, if the file doesn't exist the program will crash
	std::fstream file = std::fstream(".\\hook_fonts\\arial_22.spritefont");

	if (!file.fail())
	{
		file.close(); // Close the stream so we don't block the file
		exampleFont = std::make_shared<SpriteFont>(device.Get(), L".\\hook_fonts\\arial_22.spritefont");
	}

}

void Renderer::DrawExampleTriangle()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

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

bool Renderer::IsInitialized()
{
	return initialized;
}

bool Renderer::IsFirstRender()
{
	return firstRender;
}

int Renderer::GetWindowWidth()
{
	return windowWidth;
}

int Renderer::GetWindowHeight()
{
	return windowHeight;
}

void Renderer::SetFirstRender(bool isFirstRender)
{
	firstRender = isFirstRender;
}

ID3D11Device* Renderer::GetDevice()
{
	return device.Get();
}