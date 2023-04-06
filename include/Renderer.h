#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>
#include <fstream>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <vector>
#include <comdef.h>

#include "ID3DRenderer.h"
#include "IRenderCallback.h"
#include "Logger.h"

class Renderer : public ID3DRenderer
{
public:
	void OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags);
	void OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	void AddRenderCallback(IRenderCallback* object);
	void SetCommandQueue(ID3D12CommandQueue* commandQueue);
	Renderer* Clone();

private:
	Logger logger{"Renderer"};
	HWND window = 0;

	std::vector<IRenderCallback*> renderCallbacks;
	bool mustInitializeD3DResources = true;
	bool firstTimeInitPerformed = false;
	bool isDeviceRetrieved = false;
	bool isRunningD3D12 = false;
	bool drawExamples = false;
	bool examplesLoaded = false;
	int windowWidth = 0;
	int windowHeight = 0;
	UINT bufferIndex = 0;
	UINT bufferCount = 0;

	Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11Context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device = nullptr;
	std::atomic<ID3D12CommandQueue*> commandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D11On12Device> d3d11On12Device = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> d3d12RenderTargets;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> d3d11WrappedBackBuffers;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> d3d11RenderTargetViews;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3 = nullptr;
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch = nullptr;
	std::shared_ptr<DirectX::SpriteFont> exampleFont = nullptr;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D11_VIEWPORT viewport;

	bool InitD3DResources(IDXGISwapChain* swapChain);
	bool RetrieveD3DDeviceFromSwapChain();
	void GetSwapChainDescription();
	void GetBufferCount();
	void GetSwapchainWindowInfo();
	void CreateViewport();
	void InitD3D();
	void InitD3D11();
	void GetD3D11Context();
	void CreateSpriteBatch();
	void CreateD3D11RenderTargetView();
	void InitD3D12();
	void CreateD3D11On12Device();
	void CreateD3D12Buffers();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateD3D12RtvHeap();
	void CreateD3D12RenderTargetView(UINT bufferIndex, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	void CreateD3D11WrappedBackBuffer(UINT bufferIndex);
	void CreateD3D11RenderTargetViewWithWrappedBackBuffer(UINT bufferIndex);
	bool WaitForCommandQueueIfRunningD3D12();
	void Render();
	void PreRender();
	void RenderCallbacks();
	void PostRender();
	void ReleaseViewsBuffersAndContext();
	bool CheckSuccess(HRESULT hr);
};