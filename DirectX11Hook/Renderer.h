#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <fstream>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <fstream>
#include "DebugConsole.h"
#include "Textures.h"
#include "Fonts.h"
//#include "Overlay.h"

// ComPtr is an official smart pointer used for COM objects, DirectX objects are COM objects
// https://docs.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=vs-2019

class Renderer
{
public:
	Renderer();
	void OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags);
	void OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	void DrawExamples(bool draw);
	int GetWindowWidth();
	int GetWindowHeight();
	ID3D11Device* GetDevice();

private:
	DebugConsole console;
	//Overlay overlay;
	bool drawExamples = false;
	bool examplesLoaded = false;
	bool initialized = false;
	bool firstInit = true;
	int windowWidth, windowHeight;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mainRenderTargetView = nullptr;
	D3D11_VIEWPORT viewport;

	std::shared_ptr<DirectX::SpriteBatch> spriteBatch = nullptr;
	std::shared_ptr<DirectX::SpriteFont> exampleFont = nullptr;

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

	unsigned int numIndices = 0;

	DirectX::XMVECTOR trianglePos = { 0.0f, 0.0f, -5.0f };
	DirectX::XMFLOAT3 triangleScale = DirectX::XMFLOAT3(0.7f, 0.7f, 0.7f);
	DirectX::XMFLOAT3 triangleNDC = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	float triSpeed = 0.003f;
	float triangleVelX = triSpeed;
	float triangleVelY = -triSpeed;
	float triangleRotX = 0;
	float triangleRotY = 0;
	float triangleRotZ = 0;
	float counter = 0;

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

	bool Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	void Render();
	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void CreateExampleTriangle();
	void CreateExampleFont();
	void DrawExampleTriangle();
	void DrawExampleText();
};