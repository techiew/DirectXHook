#include "Renderer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

void Renderer::OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags)
{
	if (mustInitializeD3DResources)
	{
		if (!InitD3DResources(pThis))
		{
			return;
		}
		mustInitializeD3DResources = false;
	}

	Render();
}

void Renderer::OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	logger.Log("ResizeBuffers was called!");
	ReleaseViewsBuffersAndContext();
	mustInitializeD3DResources = true;
}

void Renderer::SetDrawExampleTriangle(bool doDraw)
{
	drawExamples = doDraw;
}

void Renderer::AddRenderCallback(IRenderCallback* object)
{
	callbackObject = object;
	callbackInitialized = false;
}

void Renderer::SetCommandQueue(ID3D12CommandQueue* commandQueue)
{
	this->commandQueue = commandQueue;
}

void Renderer::SetGetCommandQueueCallback(void (*callback)())
{
	callbackGetCommandQueue = callback;
}

bool Renderer::InitD3DResources(IDXGISwapChain* swapChain)
{
	logger.Log("Initializing D3D resources...");

	try
	{
		if (!isDeviceRetrieved)
		{
			this->swapChain = swapChain;
			isDeviceRetrieved = RetrieveD3DDeviceFromSwapChain();
		}

		if (WaitForCommandQueueIfRunningD3D12())
		{
			return false;
		}

		GetSwapChainDescription();
		GetBufferCount();
		GetSwapchainWindowInfo();
		CreateViewport();
		InitD3D();
	}
	catch (std::string errorMsg)
	{
		logger.Log(errorMsg);
		return false;
	}

	firstTimeInitPerformed = true;
	logger.Log("Successfully initialized D3D resources");
	return true;
}

bool Renderer::RetrieveD3DDeviceFromSwapChain()
{
	logger.Log("Retrieving D3D device...");

	bool d3d11DeviceRetrieved = SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)d3d11Device.GetAddressOf()));
	if (d3d11DeviceRetrieved)
	{
		logger.Log("Retrieved D3D11 device");
		return true;
	}
	
	bool d3d12DeviceRetrieved = SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D12Device), (void**)d3d12Device.GetAddressOf()));
	if (d3d12DeviceRetrieved)
	{
		logger.Log("Retrieved D3D12 device");
		isRunningD3D12 = true;
		return true;
	}

	throw("Failed to retrieve D3D device");
}

void Renderer::GetSwapChainDescription()
{
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChain->GetDesc(&swapChainDesc);
}

void Renderer::GetBufferCount()
{
	if (isRunningD3D12)
	{
		bufferCount = swapChainDesc.BufferCount;
	}
	else
	{
		bufferCount = 1;
	}
}

void Renderer::GetSwapchainWindowInfo()
{
	RECT hwndRect;
	GetClientRect(swapChainDesc.OutputWindow, &hwndRect);
	windowWidth = hwndRect.right - hwndRect.left;
	windowHeight = hwndRect.bottom - hwndRect.top;
	logger.Log("Window width: %i", windowWidth);
	logger.Log("Window height: %i", windowHeight);
	window = swapChainDesc.OutputWindow;
}

void Renderer::CreateViewport()
{
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.Width = windowWidth;
	viewport.Height = windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
}

void Renderer::InitD3D()
{
	if (!isRunningD3D12)
	{
		InitD3D11();
	}
	else
	{
		InitD3D12();
	}
}

void Renderer::InitD3D11()
{
	logger.Log("Initializing D3D11...");

	if (!firstTimeInitPerformed)
	{
		CreateD3D11Context();
		CreateSpriteBatch();
	}
	CreateD3D11RenderTargetView();

	logger.Log("Initialized D3D11");
}

void Renderer::CreateD3D11Context()
{
	d3d11Device->GetImmediateContext(&d3d11Context);
}

void Renderer::CreateSpriteBatch()
{
	spriteBatch = std::make_shared<SpriteBatch>(d3d11Context.Get());
}

void Renderer::CreateD3D11RenderTargetView()
{
	ComPtr<ID3D11Texture2D> backbuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.GetAddressOf());
	d3d11RenderTargetViews = std::vector<ComPtr<ID3D11RenderTargetView>>(1, nullptr);
	d3d11Device->CreateRenderTargetView(backbuffer.Get(), nullptr, d3d11RenderTargetViews[0].GetAddressOf());
	backbuffer.ReleaseAndGetAddressOf();
}

void Renderer::InitD3D12()
{
	logger.Log("Initializing D3D12...");
	
	if (!firstTimeInitPerformed)
	{
		CreateD3D11On12Device();
		CheckSuccess(swapChain->QueryInterface(__uuidof(IDXGISwapChain3), &swapChain3));
		CreateSpriteBatch();
	}
	CreateD3D12Buffers();

	logger.Log("Initialized D3D12");
}

bool Renderer::WaitForCommandQueueIfRunningD3D12()
{
	if (isRunningD3D12)
	{
		if (commandQueue.Get() == nullptr)
		{
			logger.Log("Waiting for command queue...");
			if (!getCommandQueueCalled && callbackGetCommandQueue != nullptr)
			{
				callbackGetCommandQueue();
				getCommandQueueCalled = true;
			}
			return true;
		}
	}
	return false;
}

void Renderer::CreateD3D11On12Device()
{
	D3D_FEATURE_LEVEL featureLevels = { D3D_FEATURE_LEVEL_11_0 };
	bool d3d11On12DeviceCreated = CheckSuccess(
		D3D11On12CreateDevice(
			d3d12Device.Get(),
			NULL,
			&featureLevels,
			1,
			reinterpret_cast<IUnknown**>(commandQueue.GetAddressOf()),
			1,
			0,
			d3d11Device.GetAddressOf(),
			d3d11Context.GetAddressOf(),
			nullptr));

	bool d3d11On12DeviceChecked = CheckSuccess(d3d11Device.As(&d3d11On12Device));

	if (!d3d11On12DeviceCreated || !d3d11On12DeviceChecked)
	{
		throw("Failed to create D3D11On12 device");
	}
}

void Renderer::CreateD3D12Buffers()
{
	d3d12RenderTargets = std::vector<ComPtr<ID3D12Resource>>(bufferCount, nullptr);
	d3d11WrappedBackBuffers = std::vector<ComPtr<ID3D11Resource>>(bufferCount, nullptr);
	d3d11RenderTargetViews = std::vector<ComPtr<ID3D11RenderTargetView>>(bufferCount, nullptr);

	ComPtr<ID3D12DescriptorHeap> rtvHeap = CreateD3D12RtvHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	UINT rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < bufferCount; i++)
	{
		CreateD3D12RenderTargetView(i, rtvHandle);
		CreateD3D11WrappedBackBuffer(i);
		CreateD3D11RenderTargetViewWithWrappedBackBuffer(i);
		rtvHandle.ptr = SIZE_T(INT64(rtvHandle.ptr) + INT64(1) * INT64(rtvDescriptorSize));
	}
}

ComPtr<ID3D12DescriptorHeap> Renderer::CreateD3D12RtvHeap()
{
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	CheckSuccess(d3d12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));
	return rtvHeap;
}

void Renderer::CreateD3D12RenderTargetView(UINT bufferIndex, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
	if (!CheckSuccess(swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&d3d12RenderTargets[bufferIndex]))))
	{
		throw("Failed to create D3D12 render target view");
	}
	d3d12Device->CreateRenderTargetView(d3d12RenderTargets[bufferIndex].Get(), nullptr, rtvHandle);
}

void Renderer::CreateD3D11WrappedBackBuffer(UINT bufferIndex)
{
	D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
	if (!CheckSuccess(
		d3d11On12Device->CreateWrappedResource(
			d3d12RenderTargets[bufferIndex].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&d3d11WrappedBackBuffers[bufferIndex]))))
	{
		throw "Failed to create D3D11 wrapped backbuffer";
	}
}

void Renderer::CreateD3D11RenderTargetViewWithWrappedBackBuffer(UINT bufferIndex)
{
	if (!CheckSuccess(
		d3d11Device->CreateRenderTargetView(
			d3d11WrappedBackBuffers[bufferIndex].Get(),
			nullptr,
			d3d11RenderTargetViews[bufferIndex].GetAddressOf())))
	{
		throw "Failed to create D3D11 render target view";
	}
}

void Renderer::Render()
{	
	PreRender();

	if (drawExamples)
	{
		if (!examplesLoaded)
		{
			CreatePipeline();
			CreateExampleTriangle();
			CreateExampleFont();
			examplesLoaded = true;
		}

		DrawExampleTriangle();
		DrawExampleText();
	}

	RenderCallbacks();
	PostRender();
}

void Renderer::PreRender()
{
	if (isRunningD3D12)
	{
		bufferIndex = swapChain3->GetCurrentBackBufferIndex();
		d3d11On12Device->AcquireWrappedResources(d3d11WrappedBackBuffers[bufferIndex].GetAddressOf(), 1);
	}

	d3d11Context->OMSetRenderTargets(1, d3d11RenderTargetViews[bufferIndex].GetAddressOf(), 0);
	d3d11Context->RSSetViewports(1, &viewport);
}

void Renderer::RenderCallbacks()
{
	if (callbackObject != nullptr)
	{
		if (!callbackInitialized)
		{
			callbackObject->Init(d3d11Device, d3d11Context, spriteBatch, window);
			callbackObject->Setup();
			callbackInitialized = true;
		}

		spriteBatch->Begin(SpriteSortMode_BackToFront);
		callbackObject->Render();
		spriteBatch->End();
	}
}

void Renderer::PostRender()
{
	if (isRunningD3D12)
	{
		d3d11On12Device->ReleaseWrappedResources(d3d11WrappedBackBuffers[bufferIndex].GetAddressOf(), 1);
		d3d11Context->Flush();
	}
}

// Creates the necessary things for rendering the examples
void Renderer::CreatePipeline()
{
	ComPtr<ID3DBlob> vertexShaderBlob = LoadShader(shaderData, "vs_5_0", "VS").Get();
	ComPtr<ID3DBlob> pixelShaderTexturesBlob = LoadShader(shaderData, "ps_5_0", "PSTex").Get();
	ComPtr<ID3DBlob> pixelShaderBlob = LoadShader(shaderData, "ps_5_0", "PS").Get();

	d3d11Device->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		nullptr, 
		vertexShader.GetAddressOf());

	d3d11Device->CreatePixelShader(
		pixelShaderTexturesBlob->GetBufferPointer(),
		pixelShaderTexturesBlob->GetBufferSize(), 
		nullptr, 
		pixelShaderTextures.GetAddressOf());

	d3d11Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[3] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	d3d11Device->CreateInputLayout(
		inputLayoutDesc,
		ARRAYSIZE(inputLayoutDesc),
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), 
		inputLayout.GetAddressOf());

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	d3d11Device->CreateSamplerState(&samplerDesc, &samplerState);

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

	d3d11Device->CreateTexture2D(&dsDesc, 0, depthStencilBuffer.GetAddressOf());
	d3d11Device->CreateDepthStencilView(depthStencilBuffer.Get(), 0, depthStencilView.GetAddressOf());
}

ComPtr<ID3DBlob> Renderer::LoadShader(const char* shader, std::string targetShaderVersion, std::string shaderEntry)
{
	logger.Log("Loading shader: %s", shaderEntry.c_str());
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ComPtr<ID3DBlob> shaderBlob;

	D3DCompile(
		shader, 
		strlen(shader), 
		0, 
		nullptr, 
		nullptr, 
		shaderEntry.c_str(), 
		targetShaderVersion.c_str(), 
		D3DCOMPILE_ENABLE_STRICTNESS, 
		0, 
		shaderBlob.GetAddressOf(), 
		errorBlob.GetAddressOf());

	if (errorBlob)
	{
		char error[256]{ 0 };
		memcpy(error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
		logger.Log("Shader error: %s", error);
		return nullptr;
	}

	return shaderBlob;
}

void Renderer::CreateExampleTriangle()
{
	// Create the vertex buffer
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

	d3d11Device->CreateBuffer(&vbDesc, &vbData, vertexBuffer.GetAddressOf());

	// Create the index buffer
	unsigned int indices[] =
	{
		0, 2, 1,
		0, 3, 2,
		0, 1, 3,
		1, 2, 3
	};

	triangleNumIndices = ARRAYSIZE(indices);

	D3D11_BUFFER_DESC ibDesc;
	ZeroMemory(&ibDesc, sizeof(ibDesc));
	ibDesc.ByteWidth = sizeof(unsigned int) * ARRAYSIZE(indices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };

	d3d11Device->CreateBuffer(&ibDesc, &ibData, indexBuffer.GetAddressOf());

	// Create the constant buffer
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

	d3d11Device->CreateBuffer(&cbDesc, &cbData, constantBuffer.GetAddressOf());

	// Create the rasterizer state.
	// We need to control which face of a shape is culled, and we need to know which order to set our indices
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	d3d11Device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf());

	// Create the depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	d3d11Device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());
}

void Renderer::CreateExampleFont()
{
	std::fstream file = std::fstream(".\\hook_fonts\\OpenSans-22.spritefont");

	if (!file.fail())
	{
		file.close();
		exampleFont = std::make_shared<SpriteFont>(d3d11Device.Get(), L".\\hook_fonts\\OpenSans-22.spritefont");
	}
	else
	{
		logger.Log("Failed to load the example font");
	}
}

void Renderer::DrawExampleTriangle()
{
	d3d11Context->OMSetRenderTargets(1, d3d11RenderTargetViews[bufferIndex].GetAddressOf(), depthStencilView.Get());
	d3d11Context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	trianglePos = XMVectorSet
	(
		XMVectorGetX(trianglePos) + triangleVelX,
		XMVectorGetY(trianglePos) + triangleVelY,
		XMVectorGetZ(trianglePos),
		1.0f
	);

	int hit = 0;

	// Check if the triangle hits an edge of the screen
	if (triangleNdc.x > 0.96f)
	{
		triangleVelX = -triangleSpeed;
		hit++;
	}
	else if (triangleNdc.x < -0.96f)
	{
		triangleVelX = triangleSpeed;
		hit++;
	}

	if (triangleNdc.y > 0.90f)
	{
		triangleVelY = -triangleSpeed;
		hit++;
	}
	else if (triangleNdc.y < -0.90f)
	{
		triangleVelY = triangleSpeed;
		hit++;
	}

	if (hit == 2)
	{
		logger.Log("Hit the corner!");
	}

	triangleCounter += 0.01f;
	triangleRotX = cos(triangleCounter) * 2;
	triangleRotY = sin(triangleCounter) * 2;

	XMMATRIX world = XMMatrixIdentity();

	XMMATRIX translation = XMMatrixTranslation(XMVectorGetX(trianglePos), XMVectorGetY(trianglePos), XMVectorGetZ(trianglePos));

	XMMATRIX rotationX = XMMatrixRotationX(triangleRotX);
	XMMATRIX rotationY = XMMatrixRotationY(triangleRotY);
	XMMATRIX rotationZ = XMMatrixRotationZ(triangleRotZ);
	XMMATRIX rotation = rotationX * rotationY * rotationZ;

	XMMATRIX scale = XMMatrixScaling(triangleScale.x, triangleScale.y, triangleScale.z);

	world = scale * rotation * translation;

	XMMATRIX view = XMMatrixLookAtLH(XMVECTOR{ 0.0f, 0.0f, -5.5f }, XMVECTOR{ 0.0f, 0.0f, 0.0f }, XMVECTOR{ 0.0f, 1.0f, 0.0f });

	XMMATRIX projection = XMMatrixPerspectiveFovLH(1.3, ((float)windowWidth / (float)windowHeight), 0.1f, 1000.0f);

	// Get the triangle's screen space (NDC) from its world space, used for collision checking and text positioning
	XMVECTOR clipSpacePos = XMVector4Transform((XMVector4Transform(trianglePos, view)), projection);

	XMStoreFloat3
	(
		&triangleNdc,
		{ 
			XMVectorGetX(clipSpacePos) / XMVectorGetW(clipSpacePos),
			XMVectorGetY(clipSpacePos) / XMVectorGetW(clipSpacePos),
			XMVectorGetZ(clipSpacePos) / XMVectorGetW(clipSpacePos) 
		}
	);

	constantBufferData.wvp = XMMatrixTranspose(world * view * projection);

	// Map the constant buffer on the GPU
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	d3d11Context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &constantBufferData, sizeof(ConstantBufferData));
	d3d11Context->Unmap(constantBuffer.Get(), 0);

	d3d11Context->VSSetShader(vertexShader.Get(), nullptr, 0);
	d3d11Context->PSSetShader(pixelShader.Get(), nullptr, 0);

	d3d11Context->IASetInputLayout(inputLayout.Get());
	d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	d3d11Context->RSSetState(rasterizerState.Get());
	d3d11Context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	d3d11Context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	d3d11Context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	d3d11Context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	d3d11Context->DrawIndexed(triangleNumIndices, 0, 0);
}

void Renderer::DrawExampleText()
{
	if (exampleFont == nullptr) return;

	const char* text = "Hello, World!";
	const char* text2 = "This is a DirectX hook.";
	XMFLOAT2 stringSize1, stringSize2;
	XMVECTOR textVector = exampleFont->MeasureString(text);
	XMVECTOR textVector2 = exampleFont->MeasureString(text2);
	XMStoreFloat2(&stringSize1, textVector);
	XMStoreFloat2(&stringSize2, textVector2);

	XMFLOAT2 textPos1 = XMFLOAT2
	(
		(windowWidth / 2) * (triangleNdc.x + 1) - (stringSize1.x / 2),
		(windowHeight - ((windowHeight / 2) * (triangleNdc.y + 1))) - (stringSize1.y / 2) - 150
	);

	XMFLOAT2 textPos2 = XMFLOAT2
	(
		(windowWidth / 2) * (triangleNdc.x + 1) - (stringSize2.x / 2),
		(windowHeight - ((windowHeight / 2) * (triangleNdc.y + 1))) - (stringSize2.y / 2) + 150
	);

	spriteBatch->Begin();
	exampleFont->DrawString(spriteBatch.get(), text, textPos1);
	exampleFont->DrawString(spriteBatch.get(), text2, textPos2);
	spriteBatch->End();
}

void Renderer::ReleaseViewsBuffersAndContext()
{
	for (int i = 0; i < bufferCount; i++)
	{
		if (d3d12Device.Get() == nullptr)
		{
			d3d11RenderTargetViews[i].ReleaseAndGetAddressOf();
		}
		else
		{
			d3d11RenderTargetViews[i].ReleaseAndGetAddressOf();
			d3d12RenderTargets[i].ReleaseAndGetAddressOf();
			d3d11WrappedBackBuffers[i].ReleaseAndGetAddressOf();
		}
	}
	
	if (d3d11Context.Get() != nullptr)
	{
		d3d11Context->Flush();
	}
}

bool Renderer::CheckSuccess(HRESULT hr)
{
	if (SUCCEEDED(hr))
	{
		return true;
	}
	_com_error err(hr);
	logger.Log("%s", err.ErrorMessage());
	return false;
}