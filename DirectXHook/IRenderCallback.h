#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>

class IRenderCallback
{
public:
	virtual void Setup() { };
	virtual void Render() = 0;
	void Init(
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<DirectX::SpriteBatch> spriteBatch,
		HWND window)
	{
		m_device = device;
		m_context = context;
		m_spriteBatch = spriteBatch;
		m_window = window;
	}

protected:
	Microsoft::WRL::ComPtr<ID3D11Device> m_device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context = nullptr;
	std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch = nullptr;
	HWND m_window;
};