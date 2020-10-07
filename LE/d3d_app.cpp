#include "d3d_app.h"

d3d_app::d3d_app()
	:
	m_depth_stencil_format(DXGI_FORMAT_D24_UNORM_S8_UINT),
	m_dxgi_format(DXGI_FORMAT_R8G8B8A8_UNORM)
{
}

d3d_app::~d3d_app()
{
	if (m_d3d_device)
		flush_command_queue();
}

void d3d_app::init(HWND hwnd, int client_width, int client_height)
{
	m_main_wnd = hwnd;
	m_client_width = client_width;
	m_client_height = client_height;

	init_d3d();

	on_resize();
}

LRESULT d3d_app::main_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return d3d_app::GetInstancePtr()->msg_proc(hwnd, msg, wParam, lParam);
}

LRESULT d3d_app::msg_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			//mAppPaused = true;
			//mTimer.Stop();
		}
		else
		{
			//mAppPaused = false;
			//mTimer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		m_client_width = LOWORD(lParam);
		m_client_height = HIWORD(lParam);
		if (m_d3d_device)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_app_paused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_app_paused = false;
				m_minimized = false;
				m_maximized = true;
				on_resize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (m_minimized)
				{
					m_app_paused = false;
					m_minimized = false;
					on_resize();
				}

				// Restoring from maximized state?
				else if (m_maximized)
				{
					m_app_paused = false;
					m_maximized = false;
					on_resize();
				}
				else if (m_resizing)
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
					on_resize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_app_paused = true;
		m_resizing = true;
		//mTimer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		m_app_paused = false;
		m_resizing = false;
		//mTimer.Start();
		on_resize();
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
		//OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		//OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		//OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
		else if ((int)wParam == VK_F2)
			//Set4xMsaaState(!m4xMsaaState);

		return 0;
	}
	
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void d3d_app::on_resize()
{
	assert(m_d3d_device);
	assert(m_swap_chain);
	assert(m_command_allocator);

	flush_command_queue();

	ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));
	// Release the previous resources we will be recreating.
	for (int i = 0; i < swap_chain_buffer_count; ++i)
		m_swap_chain_buffer[i].Reset();
	m_depth_stencil_buffer.Reset();

	// Resize the swap chain.
	ThrowIfFailed(m_swap_chain->ResizeBuffers(
		swap_chain_buffer_count,
		m_client_width, m_client_height,
		m_dxgi_format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_current_back_buffer = 0;
	create_render_resource();
}

void d3d_app::init_d3d()
{
	create_d3d_device();
	create_command_objects();
	create_swap_chain();
	create_descriptor_heap();
}

void d3d_app::create_d3d_device()
{
#ifdef DX12_ENABLE_DEBUG_LAYER
	ComPtr<ID3D12Debug> debug_controller;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debug_controller.GetAddressOf())));
	debug_controller->EnableDebugLayer();
#endif // defined(DEBUG) || defined(_DEBUG)
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgi_factory.GetAddressOf())));

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
	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_d3d_device.GetAddressOf()));
	if (FAILED(hr))
	{
		ComPtr<IDXGIAdapter> warp_adapter;
		ThrowIfFailed(m_dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(warp_adapter.GetAddressOf())));

		ThrowIfFailed(D3D12CreateDevice(warp_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_d3d_device.GetAddressOf())))
	}

	//m_d3d_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_levels_info, sizeof(feature_levels_info));

	// 创建围栏并获取资源描述符大小
	ThrowIfFailed(m_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));

	m_rtv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 检查多重采样质量级别
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS quality_level;
	quality_level.Format = m_dxgi_format;
	quality_level.SampleCount = 8;
	quality_level.NumQualityLevels = 0;
	quality_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	ThrowIfFailed(m_d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &quality_level, sizeof(quality_level)));
	m_num_quality_levels = quality_level.NumQualityLevels;
	assert(m_num_quality_levels > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	log_adapters();
#endif // _DEBUG
}

ID3D12Resource* d3d_app::current_back_buffer() const
{
	return m_swap_chain_buffer[m_current_back_buffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d_app::current_back_buffer_view() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(),
		m_current_back_buffer,
		m_rtv_descriptor_size);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d_app::depth_stencil_view() const
{
	return m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
}

void d3d_app::log_adapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapter_list;
	while (m_dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		std::wstring text = L"*** Adapter: ";
		text += desc.Description;
		text += L"\n";
		OutputDebugString(text.c_str());
		adapter_list.push_back(adapter);
		++i;
	}
	for (size_t i = 0; i < adapter_list.size(); i++)
	{
		log_adapter_outputs(adapter_list[i]);
		ReleaseCom(adapter_list[i]);
	}
}

void d3d_app::log_adapter_outputs(IDXGIAdapter* adapter)
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
		log_output_display_modes(output, DXGI_FORMAT_B8G8R8A8_UNORM);
		ReleaseCom(output);
		++i;
	}
}

void d3d_app::log_output_display_modes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> mode_list(count);
	output->GetDisplayModeList(format, flags, &count, &mode_list[0]);

	for (const auto& x : mode_list)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"width = " + std::to_wstring(x.Width) +
			L" height = " + std::to_wstring(x.Height) +
			L" refresh_rate = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";
		OutputDebugString(text.c_str());
	}
}

void d3d_app::create_command_objects()
{
	// 创建命令队列和命令列表
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_d3d_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(m_command_queue.GetAddressOf())));
	ThrowIfFailed(m_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_allocator.GetAddressOf())));
	ThrowIfFailed(m_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_command_allocator.Get(), nullptr, IID_PPV_ARGS(m_command_list.GetAddressOf())));
	m_command_list->Close();
}

void d3d_app::create_swap_chain()
{
	m_swap_chain.Reset();
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = m_client_width;
	sd.BufferDesc.Height = m_client_height;
	sd.BufferDesc.Format = m_dxgi_format;
	sd.BufferDesc.RefreshRate.Numerator = 144;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = swap_chain_buffer_count;
	sd.OutputWindow = m_main_wnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.SampleDesc.Count = m_enable_msaa ? 4 : 1;
	sd.SampleDesc.Quality = m_enable_msaa ? (m_num_quality_levels - 1) : 0;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ThrowIfFailed(m_dxgi_factory->CreateSwapChain(m_command_queue.Get(), &sd, m_swap_chain.GetAddressOf()));
}

void d3d_app::create_descriptor_heap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtv_desc;
	rtv_desc.NodeMask = 0;
	rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_desc.NumDescriptors = swap_chain_buffer_count;
	rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsv_desc;
	dsv_desc.NodeMask = 0;
	dsv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_desc.NumDescriptors = 1;
	dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf())));
}

void d3d_app::create_render_resource()
{
	// 创建渲染目标视图
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < swap_chain_buffer_count; i++)
	{
		ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(m_swap_chain_buffer[i].GetAddressOf())));
		m_d3d_device->CreateRenderTargetView(m_swap_chain_buffer[i].Get(), nullptr, rtv_heap_handle);
		rtv_heap_handle.Offset(1, m_rtv_descriptor_size);
	}

	// 创建深度/模板缓冲区及其视图
	D3D12_RESOURCE_DESC depth_stencil_desc;
	depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.Format = m_depth_stencil_format;
	depth_stencil_desc.Width = m_client_width;
	depth_stencil_desc.Height = m_client_height;
	depth_stencil_desc.Alignment = 0;
	depth_stencil_desc.DepthOrArraySize = 1;
	depth_stencil_desc.SampleDesc.Count = m_enable_msaa ? 4 : 1;;
	depth_stencil_desc.SampleDesc.Quality = m_enable_msaa ? (m_num_quality_levels - 1) : 0;
	depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE opt_clear;
	opt_clear.Format = m_depth_stencil_format;
	opt_clear.DepthStencil.Depth = 1.0f;
	opt_clear.DepthStencil.Stencil = 0;

	ThrowIfFailed(m_d3d_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depth_stencil_desc,
		D3D12_RESOURCE_STATE_COMMON,
		&opt_clear,
		IID_PPV_ARGS(m_depth_stencil_buffer.GetAddressOf())
	));
	m_d3d_device->CreateDepthStencilView(m_depth_stencil_buffer.Get(), nullptr, depth_stencil_view());


	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_depth_stencil_buffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	));

	// Execute the resize commands.
	ThrowIfFailed(m_command_list->Close());
	ID3D12CommandList* cmdsLists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	flush_command_queue();

	// Update the viewport transform to cover the client area.
	m_screen_viewport.TopLeftX = 0;
	m_screen_viewport.TopLeftY = 0;
	m_screen_viewport.Width = static_cast<float>(m_client_width);
	m_screen_viewport.Height = static_cast<float>(m_client_height);
	m_screen_viewport.MinDepth = 0.0f;
	m_screen_viewport.MaxDepth = 1.0f;

	m_scissor_rect = { 0, 0, m_client_width, m_client_height };
}

void d3d_app::flush_command_queue()
{
	m_current_fence++;
	ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), m_current_fence));
	if (m_fence->GetCompletedValue() < m_current_fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_current_fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}