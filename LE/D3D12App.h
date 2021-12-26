#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "D3D12Util.h"
#include "TSingleton.h"
#include "CDescriptorHeapWrapper.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace Microsoft::WRL;

class D3D12App : public TSingleton<D3D12App>
{
public:
	D3D12App();

	~D3D12App();

	virtual void Initialize(HWND hwnd, int clientWidth, int clientHeight);

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	inline bool IsPaused() { return mAppPaused; };

protected:
	virtual void OnResize();

	virtual void Update() = 0;

	virtual void Draw() = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }

	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }

	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	void InitD3D();

	void CreateD3D12Device();

	ID3D12Resource* CurrentBackBuffer() const;

	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void LogAdapters();

	void LogAdapterOutputs(IDXGIAdapter* adapter);

	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	void CreateCommandObjects();

	void CreateSwapChain();

	void CreateDescriptorHeap();

	void CreateRenderResource();

	void FlushCommandQueue();
protected:
	ComPtr<IDXGIFactory4> mDXGIFactory = nullptr;
	ComPtr<ID3D12Device> mD3D12Device = nullptr;
	ComPtr<ID3D12Fence> mFence = nullptr;
	DXGI_FORMAT mDXGIFormat;

	bool mEnableMSAA = false;
	UINT mMSAAQualityLevels = 0;
	UINT mSampleCount = 8;

	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<IDXGISwapChain> mSwapChain;
	int mClientWidth;
	int mClientHeight;
	HWND mMainWnd;
	static const int SwapChainBufferCount = 2;
	int mCurrentBackBuffer = 0;
	std::unique_ptr<CDescriptorHeapWrapper> mRtvHeap;
	std::unique_ptr<CDescriptorHeapWrapper> mDsvHeap;
	// ImGUI
	std::unique_ptr<CDescriptorHeapWrapper> mSrvHeap;
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	DXGI_FORMAT mDepthStencilFormat;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	ComPtr<ID3D12Resource> mMSAARenderTarget;
	ComPtr<ID3D12Resource> mMSAADepthStencilBuffer;
	std::unique_ptr<CDescriptorHeapWrapper> mMSAARtvHeap;
	std::unique_ptr<CDescriptorHeapWrapper> mMSAADsvHeap;

	UINT64 mCurrentFence = 0;
	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	bool mAppPaused = false;
	bool mMinimized = false;
	bool mMaximized = false;
	bool mResizing = false;
	bool mFullscreenState = false;
};