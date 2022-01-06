#include <WindowsX.h>
#include "D3D12App.h"
#include "GameTimer.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <iostream>

DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
	default:                                return fmt;
	}
}

D3D12App::D3D12App()
	:
	mDepthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
	mBackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
{
	CoInitialize(NULL);
}

D3D12App::~D3D12App()
{
	if (mD3D12Device)
		FlushCommandQueue();
	CoUninitialize();
}

void D3D12App::Initialize(HWND hwnd, int clientWidth, int clientHeight)
{
	mMainWnd = hwnd;
	mClientWidth = clientWidth;
	mClientHeight = clientHeight;

	InitD3D();

	OnResize();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT D3D12App::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;
	return D3D12App::GetInstancePtr()->MsgProc(hwnd, msg, wParam, lParam);
}

LRESULT D3D12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			GameTimer::GetInstancePtr()->Stop();
		}
		else
		{
			mAppPaused = false;
			GameTimer::GetInstancePtr()->Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (mD3D12Device)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		GameTimer::GetInstancePtr()->Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		GameTimer::GetInstancePtr()->Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void D3D12App::OnResize()
{
	assert(mD3D12Device);
	assert(mSwapChain);
	assert(mCommandAllocator);

	FlushCommandQueue();
	// ImGui
	ImGui_ImplDX12_InvalidateDeviceObjects();

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	DXGI_FORMAT format = NoSRGB(mBackBufferFormat);

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrentBackBuffer = 0;
	CreateRenderResource();
	// ImGui
	ImGui_ImplDX12_CreateDeviceObjects();
}

void D3D12App::InitD3D()
{
	CreateD3D12Device();
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeap();
}

void D3D12App::CreateD3D12Device()
{
#if defined(DEBUG) || defined(_DEBUG) 
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
	debugController->EnableDebugLayer();
#endif // defined(DEBUG) || defined(_DEBUG)
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(mDXGIFactory.GetAddressOf())));

	//D3D_FEATURE_LEVEL feature_levers[] =
	//{
	//	D3D_FEATURE_LEVEL_12_1,
	//	D3D_FEATURE_LEVEL_12_0,
	//	D3D_FEATURE_LEVEL_11_1,
	//	D3D_FEATURE_LEVEL_11_0,
	//	D3D_FEATURE_LEVEL_10_1,
	//	D3D_FEATURE_LEVEL_10_0,
	//	D3D_FEATURE_LEVEL_9_3,
	//	D3D_FEATURE_LEVEL_9_2,
	//	D3D_FEATURE_LEVEL_9_1
	//};

	//D3D12_FEATURE_DATA_FEATURE_LEVELS feature_levels_info;
	//feature_levels_info.NumFeatureLevels = ARRAYSIZE(feature_levers);
	//feature_levels_info.pFeatureLevelsRequested = feature_levers;

	// 创建设备
	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(mD3D12Device.GetAddressOf()));
	if (FAILED(hr))
	{
		ComPtr<IDXGIAdapter> warp_adapter;
		ThrowIfFailed(mDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(warp_adapter.GetAddressOf())));

		ThrowIfFailed(D3D12CreateDevice(warp_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(mD3D12Device.GetAddressOf())))
	}

	//m_d3d_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_levels_info, sizeof(feature_levels_info));

	// 创建围栏并获取资源描述符大小
	ThrowIfFailed(mD3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.GetAddressOf())));

	// 检查多重采样质量级别
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevel;
	qualityLevel.Format = mBackBufferFormat;
	qualityLevel.SampleCount = mSampleCount;
	qualityLevel.NumQualityLevels = 0;
	qualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	ThrowIfFailed(mD3D12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevel, sizeof(qualityLevel)));
	mMSAAQualityLevels = qualityLevel.NumQualityLevels;
	assert(mMSAAQualityLevels > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif // _DEBUG
}

ID3D12Resource* D3D12App::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::CurrentBackBufferView() const
{
	return mRtvHeap->hCPU(mCurrentBackBuffer);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::DepthStencilView() const
{
	return mDsvHeap->hCPU(0);
}

void D3D12App::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mDXGIFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		std::wstring text = L"*** Adapter: ";
		text += desc.Description;
		text += L"\n";
		OutputDebugString(text.c_str());
		std::wcout << text.c_str() << std::endl;
		adapterList.push_back(adapter);
		++i;
	}
	for (size_t i = 0; i < adapterList.size(); i++)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3D12App::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"**output**";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());
		std::wcout << text.c_str() << std::endl;
		LogOutputDisplayModes(output, DXGI_FORMAT_B8G8R8A8_UNORM);
		ReleaseCom(output);
		++i;
	}
}

void D3D12App::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (const auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"width = " + std::to_wstring(x.Width) +
			L" height = " + std::to_wstring(x.Height) +
			L" refresh_rate = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";
		OutputDebugString(text.c_str());
		std::wcout << text.c_str() << std::endl;
	}
}

void D3D12App::CreateCommandObjects()
{
	// 创建命令队列和命令列表
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(mD3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf())));
	ThrowIfFailed(mD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCommandAllocator.GetAddressOf())));
	ThrowIfFailed(mD3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));
	mCommandAllocator->SetName(L"CommandAllocator");
	mCommandList->SetName(L"MainCommandList");
	mCommandList->Close();
}

void D3D12App::CreateSwapChain()
{
	mSwapChain.Reset();
	DXGI_FORMAT format = NoSRGB(mBackBufferFormat);

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.Format = format;
	sd.BufferDesc.RefreshRate.Numerator = 144;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ThrowIfFailed(mDXGIFactory->CreateSwapChain(mCommandQueue.Get(), &sd, mSwapChain.GetAddressOf()));
}

void D3D12App::CreateDescriptorHeap()
{
	mRtvHeap = std::make_unique<CDescriptorHeapWrapper>();
	ThrowIfFailed(mRtvHeap->Create(mD3D12Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, SwapChainBufferCount, false));

	mDsvHeap = std::make_unique<CDescriptorHeapWrapper>();
	ThrowIfFailed(mDsvHeap->Create(mD3D12Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false));

	mSrvHeap = std::make_unique<CDescriptorHeapWrapper>();
	ThrowIfFailed(mSrvHeap->Create(mD3D12Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true));


	mMSAARtvHeap = std::make_unique<CDescriptorHeapWrapper>();
	ThrowIfFailed(mMSAARtvHeap->Create(mD3D12Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false));

	mMSAADsvHeap = std::make_unique<CDescriptorHeapWrapper>();
	ThrowIfFailed(mMSAADsvHeap->Create(mD3D12Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false));
}

void D3D12App::CreateRenderResource()
{
	// 创建渲染目标视图
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->hCPU(0));
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mSwapChainBuffer[i].GetAddressOf())));
		mD3D12Device->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvHeap->GetHandleIncrementSize());
	}

	// 创建深度/模板缓冲区及其视图
	D3D12_RESOURCE_DESC depth_stencil_desc = CD3DX12_RESOURCE_DESC::Tex2D(
		mDepthStencilFormat,
		mClientWidth,
		mClientHeight,
		1,
		1
	);
	depth_stencil_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClear;
	depthClear.Format = mDepthStencilFormat;
	depthClear.DepthStencil.Depth = 1.0f;
	depthClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(mD3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depth_stencil_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
	));
	mD3D12Device->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	//mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
	//	mDepthStencilBuffer.Get(),
	//	D3D12_RESOURCE_STATE_COMMON,
	//	D3D12_RESOURCE_STATE_DEPTH_WRITE
	//));

#pragma region MSAA

	mMSAARenderTarget.Reset();
	mMSAADepthStencilBuffer.Reset();

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mBackBufferFormat,
		mClientWidth,
		mClientHeight,
		1,
		1,
		mSampleCount
	);
	msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE msaaClear = {};
	msaaClear.Format = mBackBufferFormat;
	memcpy(msaaClear.Color, DirectX::Colors::DarkSlateGray, sizeof(float) * 4);

	mD3D12Device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&msaaRTDesc,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		&msaaClear,
		IID_PPV_ARGS(mMSAARenderTarget.GetAddressOf())
	);

	mMSAARenderTarget->SetName(L"MSAA Render Target");

	D3D12_RENDER_TARGET_VIEW_DESC msaaRtvDesc = {};
	msaaRtvDesc.Format = mBackBufferFormat;
	//https://docs.microsoft.com/zh-cn/windows/win32/api/d3d12/ne-d3d12-d3d12_rtv_dimension
	msaaRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE msaaRtvHeapHandle(mMSAARtvHeap->hCPU(0));
	mD3D12Device->CreateRenderTargetView(
		mMSAARenderTarget.Get(),
		&msaaRtvDesc,
		msaaRtvHeapHandle);

	D3D12_RESOURCE_DESC msaaDSDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mDepthStencilFormat,
		mClientWidth,
		mClientHeight,
		1,
		1,
		mSampleCount
	);
	msaaDSDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE msaaDepthClear;
	msaaDepthClear.Format = mDepthStencilFormat;
	msaaDepthClear.DepthStencil.Depth = 1.0f;
	msaaDepthClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(mD3D12Device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&msaaDSDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&msaaDepthClear,
		IID_PPV_ARGS(mMSAADepthStencilBuffer.GetAddressOf())
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE msaaDsvHeapHandle(mMSAADsvHeap->hCPU(0));
	mD3D12Device->CreateDepthStencilView(mMSAADepthStencilBuffer.Get(), &dsvDesc, msaaDsvHeapHandle);

#pragma endregion

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

void D3D12App::FlushCommandQueue()
{
	mCurrentFence++;
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}