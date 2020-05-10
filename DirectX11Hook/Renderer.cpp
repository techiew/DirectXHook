#include "Renderer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

// We load the shaders at compile time into a constant string.
const char* shaderData = {
#include "Shaders.hlsl"
};

Renderer::Renderer(DebugConsole* console, bool drawExamples)
{
	this->console = console;
	this->drawExamples = drawExamples;
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
	console->PrintDebugMsg("Window width: %i", (void*)windowWidth);
	console->PrintDebugMsg("Window height: %i", (void*)windowHeight);

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

		if (drawExamples)
		{
			CreatePipeline();
			CreateExampleTriangle();
			CreateExampleFont();
		}

		console->PrintDebugMsg("Successfully initialized the renderer", nullptr, MsgType::COMPLETE);

		overlay = Overlay(device.Get(), spriteBatch.get(), console, desc.OutputWindow);
	}

	overlay.SetWindowHandle(desc.OutputWindow);

	initialized = true;
	firstInit = false;
	return true;
}

void Renderer::Render(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	if (!initialized) return;

	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), 0);
	context->RSSetViewports(1, &viewport);

	if (drawExamples)
	{
		DrawExampleTriangle(); // Example triangle for testing
		DrawExampleText(); // Same but with text
	}

	overlay.Render();
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

	// Create depth stencil
	D3D11_TEXTURE2D_DESC dsDesc;
	dsDesc.Width = windowWidth;
	dsDesc.Height = windowHeight;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = 0;

	device->CreateTexture2D(&dsDesc, 0, depthStencilBuffer.GetAddressOf());
	device->CreateDepthStencilView(depthStencilBuffer.Get(), 0, depthStencilView.GetAddressOf());
}

ComPtr<ID3DBlob> Renderer::LoadShader(const char* shader, std::string targetShaderVersion, std::string shaderEntry)
{
	console->PrintDebugMsg("Loading shader: %s", (void*)shaderEntry.c_str());
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

	console->PrintDebugMsg("Shader loaded");
	return shaderBlob;
}

void Renderer::CreateExampleTriangle()
{
	// Create vertex buffer
	Vertex vertices[] =
	{
		{ XMFLOAT3(0.0f, 0.1f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.5f, 0.0f) },
		{ XMFLOAT3(0.1f, -0.1f, 0.1f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.5f) },
		{ XMFLOAT3(-0.1f, -0.1f, 0.1f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.5f) },
		{ XMFLOAT3(0.0f, -0.1f, -0.1f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.5f) }
	};

	D3D11_BUFFER_DESC vbDesc = { 0 };
	ZeroMemory(&vbDesc, sizeof(D3D11_BUFFER_DESC));
	vbDesc.ByteWidth = sizeof(Vertex) * ARRAYSIZE(vertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA vbData = { vertices, 0, 0 };

	device->CreateBuffer(&vbDesc, &vbData, vertexBuffer.GetAddressOf());

	// Create index buffer
	unsigned int indices[] =
	{
		0, 2, 1,
		0, 3, 2,
		0, 1, 3,
		1, 2, 3
	};

	numIndices = ARRAYSIZE(indices);

	D3D11_BUFFER_DESC ibDesc;
	ZeroMemory(&ibDesc, sizeof(ibDesc));
	ibDesc.ByteWidth = sizeof(unsigned int) * ARRAYSIZE(indices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };

	device->CreateBuffer(&ibDesc, &ibData, indexBuffer.GetAddressOf());

	// Create constant buffer
	// We need to send the world view projection (WVP) matrix to the shader
	D3D11_BUFFER_DESC cbDesc = { 0 };
	ZeroMemory(&cbDesc, sizeof(D3D11_BUFFER_DESC));
	cbDesc.ByteWidth = sizeof(ConstantBufferData);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbData = { &constantBufferData, 0, 0 };

	device->CreateBuffer(&cbDesc, &cbData, constantBuffer.GetAddressOf());

	// Create rasterizer state
	// We need to control which face of a shape is culled
	// And we need to know which order to set our indices
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

	// Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());
}

void Renderer::CreateExampleFont()
{
	std::fstream file = std::fstream(".\\hook_fonts\\arial_22.spritefont");

	if (!file.fail())
	{
		file.close();
		exampleFont = std::make_shared<SpriteFont>(device.Get(), L".\\hook_fonts\\arial_22.spritefont");
	}
	else
	{
		console->PrintDebugMsg("Failed to load the example font", nullptr, MsgType::FAILED);
	}

}

void Renderer::DrawExampleTriangle()
{
	context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), depthStencilView.Get());
	context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	trianglePos = XMVectorSet
	(
		XMVectorGetX(trianglePos) + triangleVelX, 
		XMVectorGetY(trianglePos) + triangleVelY, 
		XMVectorGetZ(trianglePos), 
		1.0f
	);

	int hit = 0;

	// Check if triangle hits an edge of the screen
	if (triangleNDC.x > 0.96f)
	{
		triangleVelX = -triSpeed;
		hit++;
	}
	else if (triangleNDC.x < -0.96f)
	{
		triangleVelX = triSpeed;
		hit++;
	}

	if (triangleNDC.y > 0.90f)
	{
		triangleVelY = -triSpeed;
		hit++;
	}
	else if (triangleNDC.y < -0.90f)
	{
		triangleVelY = triSpeed;
		hit++;
	}

	if (hit == 2)
	{
		console->PrintDebugMsg("Hit the corner!");
	}

	counter += 0.01f;
	triangleRotX = cos(counter) * 2;
	triangleRotY = sin(counter) * 2;

	XMMATRIX world = XMMatrixIdentity();

	XMMATRIX translation = XMMatrixTranslation(XMVectorGetX(trianglePos), XMVectorGetY(trianglePos), XMVectorGetZ(trianglePos));

	XMMATRIX rotationX = XMMatrixRotationX(triangleRotX);
	XMMATRIX rotationY = XMMatrixRotationY(triangleRotY);
	XMMATRIX rotationZ = XMMatrixRotationZ(triangleRotZ);
	XMMATRIX rotation = rotationX * rotationY * rotationZ;

	XMMATRIX scale = XMMatrixScaling(triangleScale.x, triangleScale.y, triangleScale.z);

	world = scale * rotation * translation;

	XMMATRIX view = XMMatrixLookAtLH(XMVECTOR{ 0.0f, 0.0f, -5.5f }, XMVECTOR{ 0.0f, 0.0f, 0.0f }, XMVECTOR{ 0.0f, 1.0f, 0.0f });

	XMMATRIX projection = XMMatrixPerspectiveFovLH(1.57f, ((float)windowWidth / (float)windowHeight), 0.1f, 1000.0f);

	// Get the triangle's screen space (NDC) from its world space, used for collision checking and text positioning
	// https://stackoverflow.com/questions/8491247/c-opengl-convert-world-coords-to-screen2d-coords
	XMVECTOR clipSpacePos = XMVector4Transform((XMVector4Transform(trianglePos, view)), projection);

	XMStoreFloat3
	(
		&triangleNDC, // Store screen pos to this variable
		{ 
			XMVectorGetX(clipSpacePos) / XMVectorGetW(clipSpacePos),
			XMVectorGetY(clipSpacePos) / XMVectorGetW(clipSpacePos),
			XMVectorGetZ(clipSpacePos) / XMVectorGetW(clipSpacePos) 
		}
	);

	constantBufferData.wvp = XMMatrixTranspose(world * view * projection); // Multiplication order is inverted because of the transpose

	// Map the constant buffer on the GPU
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &constantBufferData, sizeof(ConstantBufferData));
	context->Unmap(constantBuffer.Get(), 0);

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->PSSetShader(pixelShader.Get(), nullptr, 0);

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->RSSetState(rasterizerState.Get());
	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	context->UpdateSubresource(constantBuffer.Get(), 0, 0, &constantBufferData, 0, 0);
	context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	// These would be set if textures were used
	//context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	//context->PSSetShaderResources(0, 1, texture.GetAddressOf());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->DrawIndexed(numIndices, 0, 0);
}

void Renderer::DrawExampleText()
{
	if (exampleFont == nullptr) return;

	const char* text = "Hello, World!";
	const char* text2 = "This is a DirectX 11 hook.";
	XMFLOAT2 stringSize1, stringSize2;
	XMVECTOR textVector = exampleFont->MeasureString(text);
	XMVECTOR textVector2 = exampleFont->MeasureString(text2);
	XMStoreFloat2(&stringSize1, textVector);
	XMStoreFloat2(&stringSize2, textVector2);

	XMFLOAT2 textPos1 = XMFLOAT2
	(
		(windowWidth / 2) * (triangleNDC.x + 1) - (stringSize1.x / 2),
		(windowHeight - ((windowHeight / 2) * (triangleNDC.y + 1))) - (stringSize1.y / 2) - 150
	);

	XMFLOAT2 textPos2 = XMFLOAT2
	(
		(windowWidth / 2) * (triangleNDC.x + 1) - (stringSize2.x / 2),
		(windowHeight - ((windowHeight / 2) * (triangleNDC.y + 1))) - (stringSize2.y / 2) + 150
	);

	spriteBatch->Begin();
	exampleFont->DrawString(spriteBatch.get(), text, textPos1);
	exampleFont->DrawString(spriteBatch.get(), text2, textPos2);
	spriteBatch->End();
}

void Renderer::OnPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
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