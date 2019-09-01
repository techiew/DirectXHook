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

// ComPtr is an official smart pointer for COM objects, DirectX objects are COM objects

class Renderer
{
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderTextures;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context = NULL;
	Microsoft::WRL::ComPtr<ID3D11Device> device = NULL;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mainRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	D3D11_VIEWPORT viewport;
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch = nullptr;
	std::shared_ptr<DirectX::SpriteFont> exampleFont = nullptr;

	DebugConsole* console;
	Textures* textures;
	Fonts* fonts;

	bool initialized = false;
	bool firstRender = true;
	int windowWidth, windowHeight;

	void CreatePipeline();
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderData, std::string targetShaderVersion, std::string shaderEntry);
	void CreateExampleTriangle();
	void CreateExampleFont();
	void DrawExampleTriangle();
	void DrawExampleText();

public:
	Renderer() {};
	Renderer(DebugConsole* console, Textures* textures, Fonts* fonts);
	bool Init(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
	bool Render(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags, std::vector<Mesh> thingsToDraw, std::vector<Text> textToDraw, bool drawExamples);
	HRESULT CreateBufferForMesh(D3D11_BUFFER_DESC desc, D3D11_SUBRESOURCE_DATA data, ID3D11Buffer** buffer);
	bool IsInitialized();
	bool IsFirstRender();
	int GetWindowWidth();
	int GetWindowHeight();
	void SetFirstRender(bool isFirstRender);
	ID3D11Device* GetDevice();
};