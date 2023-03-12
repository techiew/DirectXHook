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

// D3D11 renderer with support for D3D12 using D3D11On12
class Renderer : public ID3DRenderer
{
public:
	void OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags);
	void OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	void SetDrawExampleTriangle(bool doDraw);
	void AddRenderCallback(IRenderCallback* object);
	void SetCommandQueue(ID3D12CommandQueue* commandQueue);
	void SetGetCommandQueueCallback(void (*callback)());

private:
	Logger logger{"Renderer"};
	HWND window = 0;

	IRenderCallback* callbackObject = nullptr;
	void (*callbackGetCommandQueue)();
	bool mustInitializeD3DResources = true;
	bool firstTimeInitPerformed = false;
	bool isDeviceRetrieved = false;
	bool isRunningD3D12 = false;
	bool getCommandQueueCalled = false;
	bool drawExamples = false;
	bool examplesLoaded = false;
	bool callbackInitialized = false;
	int windowWidth = 0;
	int windowHeight = 0;
	UINT bufferIndex = 0;
	UINT bufferCount = 0;

	Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11Context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
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

	// Load the shaders from disk at compile time into a string.
	const char* shaderData = {
		#include "Shaders.hlsl"
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderTextures = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView = nullptr;

	DirectX::XMVECTOR trianglePos = { 0.0f, 0.0f, -5.0f };
	DirectX::XMFLOAT3 triangleScale = DirectX::XMFLOAT3(0.7f, 0.7f, 0.7f);
	DirectX::XMFLOAT3 triangleNdc = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	unsigned int triangleNumIndices = 0;
	float triangleSpeed = 0.003f;
	float triangleVelX = triangleSpeed;
	float triangleVelY = -triangleSpeed;
	float triangleRotX = 0;
	float triangleRotY = 0;
	float triangleRotZ = 0;
	float triangleCounter = 0;

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texCoord;
	};

	struct ConstantBufferData
	{
		DirectX::XMMATRIX wvp = DirectX::XMMatrixIdentity();
	}
	constantBufferData;

	bool InitD3DResources(IDXGISwapChain* swapChain);
	bool RetrieveD3DDeviceFromSwapChain();
	void GetSwapChainDescription();
	void GetBufferCount();
	void GetSwapchainWindowInfo();
	void CreateViewport();
	void InitD3D();
	void InitD3D11();
	void CreateD3D11Context();
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
	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void CreateExampleTriangle();
	void CreateExampleFont();
	void DrawExampleTriangle();
	void DrawExampleText();
	void ReleaseViewsBuffersAndContext();
	bool CheckSuccess(HRESULT hr);
};