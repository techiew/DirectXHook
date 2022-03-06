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

#include "IRenderCallback.h"
#include "Logger.h"

class Renderer
{
public:
	bool missingCommandQueue = true;

	void OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags);
	void OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	void DrawExampleTriangle(bool doDraw);
	void SetRenderCallback(IRenderCallback* object);
	void SetCommandQueue(ID3D12CommandQueue* commandQueue);

private:
	Logger m_logger{ "Renderer" };
	HWND m_window = 0;
	IRenderCallback* m_callbackObject = nullptr;
	bool m_firstInit = true;
	bool m_resizeBuffers = false;
	bool m_drawExamples = false;
	bool m_examplesLoaded = false;
	bool m_callbackInitialized = false;
	int m_windowWidth = 0;
	int m_windowHeight = 0;
	UINT m_bufferIndex = 0;
	UINT m_bufferCount = 0;

	Microsoft::WRL::ComPtr<ID3D12Device> m_d3d12Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3d11Context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> m_d3d11Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D11On12Device> m_d3d11On12Device = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_d3d12RenderTargets;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> m_d3d11WrappedBackBuffers;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> m_d3d11RenderTargetViews;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3 = nullptr;
	std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch = nullptr;
	std::shared_ptr<DirectX::SpriteFont> m_exampleFont = nullptr;
	D3D11_VIEWPORT m_viewport;

	// We load the shaders from disk at compile time into a string.
	const char* m_shaderData = {
	#include "Shaders.hlsl"
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShaderTextures = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencilBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView = nullptr;

	DirectX::XMVECTOR m_trianglePos = { 0.0f, 0.0f, -5.0f };
	DirectX::XMFLOAT3 m_triangleScale = DirectX::XMFLOAT3(0.7f, 0.7f, 0.7f);
	DirectX::XMFLOAT3 m_triangleNdc = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	unsigned int m_triangleNumIndices = 0;
	float m_triangleSpeed = 0.003f;
	float m_triangleVelX = m_triangleSpeed;
	float m_triangleVelY = -m_triangleSpeed;
	float m_triangleRotX = 0;
	float m_triangleRotY = 0;
	float m_triangleRotZ = 0;
	float m_triangleCounter = 0;

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
	m_constantBufferData;

	bool Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	void Render();
	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void CreateExampleTriangle();
	void CreateExampleFont();
	void DrawExampleTriangle();
	void DrawExampleText();
	void PrintHresultError(HRESULT hr);
};