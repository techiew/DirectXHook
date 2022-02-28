#include "Renderer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

bool Renderer::Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	if (m_firstInit)
	{
		m_logger.Log("Initializing renderer...");

		if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)m_d3d11Device.GetAddressOf())))
		{
			m_d3d11Device->GetImmediateContext(&m_d3d11Context);
			m_spriteBatch = std::make_shared<SpriteBatch>(m_d3d11Context.Get());
			m_logger.Log("Getting D3D11 device succeeded");
		}
		else if (!missingCommandQueue && SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D12Device), (void**)m_d3d12Device.GetAddressOf())))
		{
			D3D_FEATURE_LEVEL featureLevels = { D3D_FEATURE_LEVEL_11_0 };
			PrintHresultError(D3D11On12CreateDevice(m_d3d12Device.Get(), NULL, &featureLevels, 1, reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()), 1, 0, &m_d3d11Device, &m_d3d11Context, nullptr));
			PrintHresultError(m_d3d11Device.As(&m_d3d11On12Device));
			PrintHresultError(swapChain->QueryInterface(__uuidof(IDXGISwapChain3), &swapChain3));
			m_spriteBatch = std::make_shared<SpriteBatch>(m_d3d11Context.Get());
			m_logger.Log("Getting D3D12 device succeeded");
		}
		else
		{
			return false;
		}
	}
	else
	{
		m_logger.Log("Resizing buffers...");
	}

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChain->GetDesc(&desc);
	if (m_d3d12Device.Get() == nullptr)
	{
		m_bufferCount = 1;
	}
	else
	{
		m_bufferCount = desc.BufferCount;
	}
	
	RECT hwndRect;
	GetClientRect(desc.OutputWindow, &hwndRect);
	m_windowWidth = hwndRect.right - hwndRect.left;
	m_windowHeight = hwndRect.bottom - hwndRect.top;
	m_logger.Log("Window width: %i", m_windowWidth);
	m_logger.Log("Window height: %i", m_windowHeight);
	m_window = desc.OutputWindow;

	ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
	m_viewport.Width = m_windowWidth;
	m_viewport.Height = m_windowHeight;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	if (m_d3d12Device.Get() == nullptr)
	{
		ComPtr<ID3D11Texture2D> backbuffer;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.GetAddressOf());
		m_d3d11RenderTargetViews = std::vector<ComPtr<ID3D11RenderTargetView>>(1, nullptr);
		m_d3d11Device->CreateRenderTargetView(backbuffer.Get(), nullptr, m_d3d11RenderTargetViews[0].GetAddressOf());
		backbuffer.ReleaseAndGetAddressOf();
	}
	else
	{
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = m_bufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		PrintHresultError(m_d3d12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));
		UINT rtvDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

		m_d3d12RenderTargets = std::vector<ComPtr<ID3D12Resource>>(m_bufferCount, nullptr);
		m_d3d11WrappedBackBuffers = std::vector<ComPtr<ID3D11Resource>>(m_bufferCount, nullptr);
		m_d3d11RenderTargetViews = std::vector<ComPtr<ID3D11RenderTargetView>>(m_bufferCount, nullptr);

		for (UINT i = 0; i < m_bufferCount; i++)
		{
			PrintHresultError(swapChain->GetBuffer(i, IID_PPV_ARGS(&m_d3d12RenderTargets[i])));
			m_d3d12Device->CreateRenderTargetView(m_d3d12RenderTargets[i].Get(), nullptr, rtvHandle);

			D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
			PrintHresultError(m_d3d11On12Device->CreateWrappedResource(
				m_d3d12RenderTargets[i].Get(),
				&d3d11Flags,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT,
				IID_PPV_ARGS(&m_d3d11WrappedBackBuffers[i])));

			PrintHresultError(m_d3d11Device->CreateRenderTargetView(m_d3d11WrappedBackBuffers[i].Get(), nullptr, m_d3d11RenderTargetViews[i].GetAddressOf()));

			rtvHandle.ptr = SIZE_T(INT64(rtvHandle.ptr) + INT64(1) * INT64(rtvDescriptorSize));
		}
	}

	if (m_firstInit)
	{
		m_logger.Log("Successfully initialized the renderer");
		m_logger.Log("Now rendering...");
	}

	m_firstInit = false;
	m_resizeBuffers = false;
	return true;
}

void Renderer::Render()
{	
	if (m_d3d12Device.Get() == nullptr)
	{
		m_bufferIndex = 0;
	}
	else
	{
		m_bufferIndex = swapChain3->GetCurrentBackBufferIndex();
		m_d3d11On12Device->AcquireWrappedResources(m_d3d11WrappedBackBuffers[m_bufferIndex].GetAddressOf(), 1);
	}

	m_d3d11Context->OMSetRenderTargets(1, m_d3d11RenderTargetViews[m_bufferIndex].GetAddressOf(), 0);
	m_d3d11Context->RSSetViewports(1, &m_viewport);

	if (m_drawExamples)
	{
		if (!m_examplesLoaded)
		{
			CreatePipeline();
			CreateExampleTriangle();
			CreateExampleFont();
			m_examplesLoaded = true;
		}

		DrawExampleTriangle();
		DrawExampleText();
	}

	if (m_callbackObject != nullptr)
	{
		if (!m_callbackInitialized)
		{
			m_callbackObject->Init(m_d3d11Device, m_d3d11Context, m_spriteBatch, m_window);
			m_callbackObject->Setup();
			m_callbackInitialized = true;
		}

		m_spriteBatch->Begin(SpriteSortMode_BackToFront);
		m_callbackObject->Render();
		m_spriteBatch->End();
	}

	if (m_d3d12Device.Get() != nullptr)
	{
		m_d3d11On12Device->ReleaseWrappedResources(m_d3d11WrappedBackBuffers[m_bufferIndex].GetAddressOf(), 1);
		m_d3d11Context->Flush();
	}
}

// Creates the necessary things for rendering the examples.
void Renderer::CreatePipeline()
{
	ComPtr<ID3DBlob> vertexShaderBlob = LoadShader(m_shaderData, "vs_5_0", "VS").Get();
	ComPtr<ID3DBlob> pixelShaderTexturesBlob = LoadShader(m_shaderData, "ps_5_0", "PSTex").Get();
	ComPtr<ID3DBlob> pixelShaderBlob = LoadShader(m_shaderData, "ps_5_0", "PS").Get();

	m_d3d11Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());

	m_d3d11Device->CreatePixelShader(pixelShaderTexturesBlob->GetBufferPointer(),
		pixelShaderTexturesBlob->GetBufferSize(), nullptr, m_pixelShaderTextures.GetAddressOf());

	m_d3d11Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[3] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	m_d3d11Device->CreateInputLayout(inputLayoutDesc, ARRAYSIZE(inputLayoutDesc), vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), m_inputLayout.GetAddressOf());

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	m_d3d11Device->CreateSamplerState(&samplerDesc, &m_samplerState);

	// Create the depth stencil.
	D3D11_TEXTURE2D_DESC dsDesc;
	dsDesc.Width = m_windowWidth;
	dsDesc.Height = m_windowHeight;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = 0;

	m_d3d11Device->CreateTexture2D(&dsDesc, 0, m_depthStencilBuffer.GetAddressOf());
	m_d3d11Device->CreateDepthStencilView(m_depthStencilBuffer.Get(), 0, m_depthStencilView.GetAddressOf());
}

ComPtr<ID3DBlob> Renderer::LoadShader(const char* shader, std::string targetShaderVersion, std::string shaderEntry)
{
	m_logger.Log("Loading shader: %s", shaderEntry.c_str());
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ComPtr<ID3DBlob> shaderBlob;

	D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, shaderEntry.c_str(), targetShaderVersion.c_str(), D3DCOMPILE_ENABLE_STRICTNESS, 0, shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
	{
		char error[256]{ 0 };
		memcpy(error, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
		m_logger.Log("Shader error: %s", error);
		return nullptr;
	}

	return shaderBlob;
}

void Renderer::CreateExampleTriangle()
{
	// Create the vertex buffer.
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

	m_d3d11Device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());

	// Create the index buffer.
	unsigned int indices[] =
	{
		0, 2, 1,
		0, 3, 2,
		0, 1, 3,
		1, 2, 3
	};

	m_triangleNumIndices = ARRAYSIZE(indices);

	D3D11_BUFFER_DESC ibDesc;
	ZeroMemory(&ibDesc, sizeof(ibDesc));
	ibDesc.ByteWidth = sizeof(unsigned int) * ARRAYSIZE(indices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };

	m_d3d11Device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());

	// Create the constant buffer.
	// We need to send the world view projection (WVP) matrix to the shader.
	D3D11_BUFFER_DESC cbDesc = { 0 };
	ZeroMemory(&cbDesc, sizeof(D3D11_BUFFER_DESC));
	cbDesc.ByteWidth = sizeof(ConstantBufferData);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbData = { &m_constantBufferData, 0, 0 };

	m_d3d11Device->CreateBuffer(&cbDesc, &cbData, m_constantBuffer.GetAddressOf());

	// Create the rasterizer state.
	// We need to control which face of a shape is culled, and we need to know which order to set our indices.
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	m_d3d11Device->CreateRasterizerState(&rsDesc, m_rasterizerState.GetAddressOf());

	// Create the depth stencil state.
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	m_d3d11Device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
}

void Renderer::CreateExampleFont()
{
	std::fstream file = std::fstream(".\\hook_fonts\\OpenSans-22.spritefont");

	if (!file.fail())
	{
		file.close();
		m_exampleFont = std::make_shared<SpriteFont>(m_d3d11Device.Get(), L".\\hook_fonts\\OpenSans-22.spritefont");
	}
	else
	{
		m_logger.Log("Failed to load the example font");
	}
}

void Renderer::DrawExampleTriangle()
{
	m_d3d11Context->OMSetRenderTargets(1, m_d3d11RenderTargetViews[m_bufferIndex].GetAddressOf(), m_depthStencilView.Get());
	m_d3d11Context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_trianglePos = XMVectorSet
	(
		XMVectorGetX(m_trianglePos) + m_triangleVelX,
		XMVectorGetY(m_trianglePos) + m_triangleVelY,
		XMVectorGetZ(m_trianglePos),
		1.0f
	);

	int hit = 0;

	// Check if the triangle hits an edge of the screen.
	if (m_triangleNdc.x > 0.96f)
	{
		m_triangleVelX = -m_triangleSpeed;
		hit++;
	}
	else if (m_triangleNdc.x < -0.96f)
	{
		m_triangleVelX = m_triangleSpeed;
		hit++;
	}

	if (m_triangleNdc.y > 0.90f)
	{
		m_triangleVelY = -m_triangleSpeed;
		hit++;
	}
	else if (m_triangleNdc.y < -0.90f)
	{
		m_triangleVelY = m_triangleSpeed;
		hit++;
	}

	if (hit == 2)
	{
		m_logger.Log("Hit the corner!"); // For some reason this prints twice sometimes?
	}

	m_triangleCounter += 0.01f;
	m_triangleRotX = cos(m_triangleCounter) * 2;
	m_triangleRotY = sin(m_triangleCounter) * 2;

	XMMATRIX world = XMMatrixIdentity();

	XMMATRIX translation = XMMatrixTranslation(XMVectorGetX(m_trianglePos), XMVectorGetY(m_trianglePos), XMVectorGetZ(m_trianglePos));

	XMMATRIX rotationX = XMMatrixRotationX(m_triangleRotX);
	XMMATRIX rotationY = XMMatrixRotationY(m_triangleRotY);
	XMMATRIX rotationZ = XMMatrixRotationZ(m_triangleRotZ);
	XMMATRIX rotation = rotationX * rotationY * rotationZ;

	XMMATRIX scale = XMMatrixScaling(m_triangleScale.x, m_triangleScale.y, m_triangleScale.z);

	world = scale * rotation * translation;

	XMMATRIX view = XMMatrixLookAtLH(XMVECTOR{ 0.0f, 0.0f, -5.5f }, XMVECTOR{ 0.0f, 0.0f, 0.0f }, XMVECTOR{ 0.0f, 1.0f, 0.0f });

	XMMATRIX projection = XMMatrixPerspectiveFovLH(1.3, ((float)m_windowWidth / (float)m_windowHeight), 0.1f, 1000.0f);

	// Get the triangle's screen space (NDC) from its world space, used for collision checking and text positioning
	XMVECTOR clipSpacePos = XMVector4Transform((XMVector4Transform(m_trianglePos, view)), projection);

	XMStoreFloat3
	(
		&m_triangleNdc,
		{ 
			XMVectorGetX(clipSpacePos) / XMVectorGetW(clipSpacePos),
			XMVectorGetY(clipSpacePos) / XMVectorGetW(clipSpacePos),
			XMVectorGetZ(clipSpacePos) / XMVectorGetW(clipSpacePos) 
		}
	);

	m_constantBufferData.wvp = XMMatrixTranspose(world * view * projection); // Multiplication order is inverted because of the transpose

	// Map the constant buffer on the GPU
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_d3d11Context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &m_constantBufferData, sizeof(ConstantBufferData));
	m_d3d11Context->Unmap(m_constantBuffer.Get(), 0);

	m_d3d11Context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3d11Context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	m_d3d11Context->IASetInputLayout(m_inputLayout.Get());
	m_d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_d3d11Context->RSSetState(m_rasterizerState.Get());
	m_d3d11Context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

	m_d3d11Context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	m_d3d11Context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3d11Context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3d11Context->DrawIndexed(m_triangleNumIndices, 0, 0);
}

void Renderer::DrawExampleText()
{
	if (m_exampleFont == nullptr) return;

	const char* text = "Hello, World!";
	const char* text2 = "This is a DirectX hook.";
	XMFLOAT2 stringSize1, stringSize2;
	XMVECTOR textVector = m_exampleFont->MeasureString(text);
	XMVECTOR textVector2 = m_exampleFont->MeasureString(text2);
	XMStoreFloat2(&stringSize1, textVector);
	XMStoreFloat2(&stringSize2, textVector2);

	XMFLOAT2 textPos1 = XMFLOAT2
	(
		(m_windowWidth / 2) * (m_triangleNdc.x + 1) - (stringSize1.x / 2),
		(m_windowHeight - ((m_windowHeight / 2) * (m_triangleNdc.y + 1))) - (stringSize1.y / 2) - 150
	);

	XMFLOAT2 textPos2 = XMFLOAT2
	(
		(m_windowWidth / 2) * (m_triangleNdc.x + 1) - (stringSize2.x / 2),
		(m_windowHeight - ((m_windowHeight / 2) * (m_triangleNdc.y + 1))) - (stringSize2.y / 2) + 150
	);

	m_spriteBatch->Begin();
	m_exampleFont->DrawString(m_spriteBatch.get(), text, textPos1);
	m_exampleFont->DrawString(m_spriteBatch.get(), text2, textPos2);
	m_spriteBatch->End();
}

void Renderer::OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags)
{
	if (m_firstInit || m_resizeBuffers)
	{
		if (!Init(pThis, syncInterval, flags))
		{
			return;
		}
	}

	Render();
}

void Renderer::OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
	m_logger.Log("ResizeBuffers was called!");
	m_resizeBuffers = true;

	for (int i = 0; i < m_bufferCount; i++)
	{
		if (m_d3d12Device.Get() == nullptr)
		{
			m_d3d11RenderTargetViews[i].ReleaseAndGetAddressOf();
		}
		else
		{
			m_d3d11RenderTargetViews[i].ReleaseAndGetAddressOf();
			m_d3d12RenderTargets[i].ReleaseAndGetAddressOf();
			m_d3d11WrappedBackBuffers[i].ReleaseAndGetAddressOf();
		}
	}

	if (m_d3d11Context.Get() != nullptr)
	{
		m_d3d11Context->Flush();
	}
}

void Renderer::DrawExamples(bool doDraw)
{
	m_drawExamples = doDraw;
}

void Renderer::SetRenderCallback(IRenderCallback* object)
{
	m_callbackObject = object;
	m_callbackInitialized = false;
}

void Renderer::SetCommandQueue(ID3D12CommandQueue* commandQueue)
{
	m_commandQueue = commandQueue;
	missingCommandQueue = false;
}

void Renderer::PrintHresultError(HRESULT hr)
{
	if(SUCCEEDED(hr))
	{
		return;
	}
	_com_error err(hr);
	m_logger.Log("%s", err.ErrorMessage());
}
