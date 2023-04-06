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

void Renderer::AddRenderCallback(IRenderCallback* callback)
{
	renderCallbacks.push_back(callback);
}

void Renderer::SetCommandQueue(ID3D12CommandQueue* commandQueue)
{
	this->commandQueue = commandQueue;
}

Renderer* Renderer::Clone()
{
	return new Renderer(*this);
}

bool Renderer::InitD3DResources(IDXGISwapChain* swapChain)
{
	if (!firstTimeInitPerformed)
	{
		logger.Log("Initializing D3D resources...");
	}
	else
	{
		logger.Log("Resizing buffers...");
	}

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
	catch (std::runtime_error e)
	{
		logger.Log("Exception: %s", e.what());
		return false;
	}

	firstTimeInitPerformed = true;
	logger.Log("Successfully initialized D3D resources");
	return true;
}

bool Renderer::RetrieveD3DDeviceFromSwapChain()
{
	logger.Log("Retrieving D3D device...");

	bool d3d12DeviceRetrieved = SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(d3d12Device.GetAddressOf())));
	if (d3d12DeviceRetrieved)
	{
		logger.Log("Retrieved D3D12 device");
		isRunningD3D12 = true;
		return true;
	}

	bool d3d11DeviceRetrieved = SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(d3d11Device.GetAddressOf())));
	if (d3d11DeviceRetrieved)
	{
		logger.Log("Retrieved D3D11 device");
		isRunningD3D12 = false;
		return true;
	}

	throw std::runtime_error("Failed to retrieve D3D device");
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

void Renderer::GetD3D11Context()
{
	logger.Log("Creating D3D11 context...");
	d3d11Device->GetImmediateContext(&d3d11Context);
}

void Renderer::CreateSpriteBatch()
{
	logger.Log("Creating spritebatch...");
	spriteBatch = std::make_shared<SpriteBatch>(d3d11Context.Get());
}

void Renderer::CreateD3D11RenderTargetView()
{
	logger.Log("Creating D3D11 render target view...");
	ComPtr<ID3D11Texture2D> backbuffer;
	swapChain->GetBuffer(0, IID_PPV_ARGS(backbuffer.GetAddressOf()));
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
		CheckSuccess(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)));
		CreateSpriteBatch();
	}
	CreateD3D12Buffers();

	logger.Log("Initialized D3D12");
}

bool Renderer::WaitForCommandQueueIfRunningD3D12()
{
	if (isRunningD3D12)
	{
		if (commandQueue == nullptr)
		{
			logger.Log("Waiting for command queue...");
			return true;
		}
	}
	return false;
}

void Renderer::CreateD3D11On12Device()
{
	logger.Log("Creating D3D11On12Device...");

	D3D_FEATURE_LEVEL featureLevels = { D3D_FEATURE_LEVEL_11_0 };
	bool d3d11On12DeviceCreated = CheckSuccess(
		D3D11On12CreateDevice(
			d3d12Device.Get(),
			NULL,
			&featureLevels,
			1,
			reinterpret_cast<IUnknown**>(&commandQueue),
			1,
			0,
			d3d11Device.GetAddressOf(),
			d3d11Context.GetAddressOf(),
			nullptr));

	bool d3d11On12DeviceChecked = CheckSuccess(d3d11Device.As(&d3d11On12Device));

	if (!d3d11On12DeviceCreated || !d3d11On12DeviceChecked)
	{
		throw std::runtime_error("Failed to create D3D11On12 device");
	}
}

void Renderer::CreateD3D12Buffers()
{
	logger.Log("Creating D3D12 buffers...");
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
	logger.Log("Creating D3D12 RTV heap...");
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
	logger.Log("Creating D3D12 render target view...");
	if (!CheckSuccess(swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&d3d12RenderTargets[bufferIndex]))))
	{
		throw std::runtime_error("Failed to create D3D12 render target view");
	}
	d3d12Device->CreateRenderTargetView(d3d12RenderTargets[bufferIndex].Get(), nullptr, rtvHandle);
}

void Renderer::CreateD3D11WrappedBackBuffer(UINT bufferIndex)
{
	logger.Log("Creating D3D11 wrapped backbuffer...");
	D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
	if (!CheckSuccess(
		d3d11On12Device->CreateWrappedResource(
			d3d12RenderTargets[bufferIndex].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&d3d11WrappedBackBuffers[bufferIndex]))))
	{
		throw std::runtime_error("Failed to create D3D11 wrapped backbuffer");
	}
}

void Renderer::CreateD3D11RenderTargetViewWithWrappedBackBuffer(UINT bufferIndex)
{
	logger.Log("Creating D3D11 render target view with wrapped backbuffer...");
	if (!CheckSuccess(
		d3d11Device->CreateRenderTargetView(
			d3d11WrappedBackBuffers[bufferIndex].Get(),
			nullptr,
			d3d11RenderTargetViews[bufferIndex].GetAddressOf())))
	{
		throw std::runtime_error("Failed to create D3D11 render target view");
	}
}

void Renderer::Render()
{	
	PreRender();
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
	for (auto& callback : renderCallbacks)
	{
		if (!callback->initialized)
		{
			callback->Init(d3d11Device, d3d11Context, spriteBatch, window);
			callback->Setup();
			callback->initialized = true;
		}

		spriteBatch->Begin(SpriteSortMode_BackToFront);
		callback->Render();
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

void Renderer::ReleaseViewsBuffersAndContext()
{
	if (isRunningD3D12)
	{
		for (auto& rtv : d3d11RenderTargetViews)
		{
			rtv.ReleaseAndGetAddressOf();
		}
		for (auto& rt : d3d12RenderTargets)
		{
			rt.ReleaseAndGetAddressOf();
		}
		for (auto& wbb : d3d11WrappedBackBuffers)
		{
			wbb.ReleaseAndGetAddressOf();
		}
	}
	else
	{
		for (auto& rtv : d3d11RenderTargetViews)
		{
			rtv.ReleaseAndGetAddressOf();
		}
	}
	
	if (d3d11Context.Get() != nullptr)
	{
		d3d11Context->Flush();
	}
}

bool Renderer::CheckSuccess(HRESULT hr)
{
	logger.Log("HRESULT error: %s", err.ErrorMessage());
	{
		return true;
	}
	_com_error err(hr);
	logger.Log("HRESULT error: %s", err.ErrorMessage());
	return false;
}