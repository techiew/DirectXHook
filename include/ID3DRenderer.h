#pragma once

#include <d3d11.h>

#include "IRenderCallback.h"

class ID3DRenderer
{
public:
	virtual void OnPresent(IDXGISwapChain* pThis, UINT syncInterval, UINT flags) = 0;
	virtual void OnResizeBuffers(IDXGISwapChain* pThis, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) {};
	virtual void AddRenderCallback(IRenderCallback* object) {};
	virtual void SetCommandQueue(ID3D12CommandQueue* commandQueue) {};
	virtual void SetGetCommandQueueCallback(void (*callback)()) {};
};