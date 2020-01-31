#pragma once

#include <Windows.h>
#include <d3d11.h>
#include "DebugConsole.h"
#include <fstream>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Vertex.h"
#include "Mesh.h"
#include "TexturedBox.h"
#include "Textures.h"
#include <wrl/client.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include "Text.h"
#include <fstream>
#include "Fonts.h"
#include "Overlay.h"

// ComPtr is an official smart pointer used for COM objects, DirectX objects are COM objects
// https://docs.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=vs-2019

class Renderer
{
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderTextures = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mainRenderTargetView = nullptr;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState = nullptr;
	D3D11_VIEWPORT viewport;

	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	std::shared_ptr<DirectX::SpriteFont> exampleFont;
	DebugConsole* console;
	Textures textures;
	Fonts fonts;
	Overlay overlay;

	bool initialized = false;
	bool firstInit = true;
	bool drawExamples = false;
	int windowWidth, windowHeight;

	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void CreateExampleTriangle();
	void CreateExampleFont();
	void DrawExampleTriangle();
	void DrawExampleText();

public:
	Renderer() {};
	Renderer(DebugConsole* console, bool drawExamples);
	bool Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	void Render(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	HRESULT CreateBufferForMesh(D3D11_BUFFER_DESC desc, D3D11_SUBRESOURCE_DATA data, ID3D11Buffer** buffer);
	void OnPresent(IDXGISwapChain *swapChain, UINT syncInterval, UINT flags);
	void OnResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
	void SetDrawExamples(bool doDraw);
	Textures* GetTextures();
	Fonts* GetFonts();
	bool IsInitialized();
	int GetWindowWidth();
	int GetWindowHeight();
	ID3D11Device* GetDevice();
};