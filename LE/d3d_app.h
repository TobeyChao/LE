#pragma once
#include "d3d_util.h"
#include "TSingleton.h"

using namespace Microsoft::WRL;

class d3d_app : public TSingleton<d3d_app>
{
public:
	d3d_app();

	~d3d_app();

	void init(HWND hwnd, int client_width, int client_height);

	static LRESULT CALLBACK main_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual LRESULT msg_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
protected:
	virtual void on_resize();

	virtual void update() = 0;

	virtual void draw() = 0;

protected:
	void init_d3d();

	void create_d3d_device();

	ID3D12Resource* current_back_buffer() const;

	D3D12_CPU_DESCRIPTOR_HANDLE current_back_buffer_view() const;

	D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view() const;

	void log_adapters();

	void log_adapter_outputs(IDXGIAdapter* adapter);

	void log_output_display_modes(IDXGIOutput* output, DXGI_FORMAT format);

	void create_command_objects();

	void create_swap_chain();

	void create_descriptor_heap();

	void create_render_resource();

	void flush_command_queue();
protected:
	ComPtr<IDXGIFactory4> m_dxgi_factory = nullptr;
	ComPtr<ID3D12Device> m_d3d_device = nullptr;
	ComPtr<ID3D12Fence> m_fence = nullptr;
	UINT m_rtv_descriptor_size = 0;
	UINT m_dsv_descriptor_size = 0;
	UINT m_cbv_descriptor_size = 0;
	DXGI_FORMAT m_dxgi_format;
	bool m_enable_msaa = false;
	UINT m_num_quality_levels = 0;
	ComPtr<ID3D12CommandAllocator> m_command_allocator;
	ComPtr<ID3D12CommandQueue> m_command_queue;
	ComPtr<ID3D12GraphicsCommandList> m_command_list;
	ComPtr<IDXGISwapChain> m_swap_chain;
	int m_client_width;
	int m_client_height;
	HWND m_main_wnd;
	static const int swap_chain_buffer_count = 2;
	int m_current_back_buffer = 0;
	ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
	ComPtr<ID3D12DescriptorHeap> m_dsv_heap;
	ComPtr<ID3D12Resource> m_swap_chain_buffer[swap_chain_buffer_count];
	DXGI_FORMAT m_depth_stencil_format;
	ComPtr<ID3D12Resource> m_depth_stencil_buffer;
	UINT64 m_current_fence = 0;
	D3D12_VIEWPORT m_screen_viewport;
	D3D12_RECT m_scissor_rect;

	bool m_app_paused = false;
	bool m_minimized = false;
	bool m_maximized = false;
	bool m_resizing = false;
	bool m_fullscreen_state = false;
};