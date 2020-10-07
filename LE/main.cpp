#include "d3d_app.h"
#include <DirectXColors.h>

using namespace DirectX;

class demo : public d3d_app
{
public:

	demo()
	{
	}

	~demo()
	{
	}

	void on_resize() override
	{
		d3d_app::on_resize();
	}

	void update() override
	{

	}

	void draw() override
	{
		// Rendering
		ThrowIfFailed(m_command_allocator->Reset());

		m_command_list->Reset(m_command_allocator.Get(), nullptr);

		m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		m_command_list->RSSetViewports(1, &m_screen_viewport);
		m_command_list->RSSetScissorRects(1, &m_scissor_rect);

		m_command_list->ClearRenderTargetView(current_back_buffer_view(), Colors::LightSteelBlue, 0, nullptr);
		m_command_list->ClearDepthStencilView(depth_stencil_view(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		m_command_list->OMSetRenderTargets(1, &current_back_buffer_view(), true, &depth_stencil_view());

		m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		ThrowIfFailed(m_command_list->Close());

		ID3D12CommandList* cmdsLists[] = { m_command_list.Get() };
		m_command_queue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		//m_swap_chain->Present(1, 0); // Present with vsync
		m_swap_chain->Present(0, 0); // Present without vsync
		m_current_back_buffer = (m_current_back_buffer + 1) % swap_chain_buffer_count;

		flush_command_queue();
	}

private:

};

// Main code
int main(int, char**)
{
	// Initialize Direct3D
	nullptr;
	try
	{
		demo* app = new demo();

		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = demo::main_wnd_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = 0;
		wc.lpszClassName = L"MainWnd";

		if (!RegisterClass(&wc))
		{
			MessageBox(0, L"RegisterClass Failed.", 0, 0);
			return false;
		}

		// Compute window rectangle dimensions based on requested client area dimensions.
		RECT R = { 0, 0, 800, 600 };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;

		HWND hwnd = CreateWindow(wc.lpszClassName, L"Dear ImGui DirectX12 Example",
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, wc.hInstance, 0);

		// Create application window
		//WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, demo::main_wnd_proc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
		//::RegisterClassEx(&wc);
		//HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Dear ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
		app->init(hwnd, 800, 600);

		// Show the window
		::ShowWindow(hwnd, SW_SHOW);
		::UpdateWindow(hwnd);

		// Main loop
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (msg.message != WM_QUIT)
		{
			if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				continue;
			}
			app->update();
			app->draw();
		}

		delete app;
		::DestroyWindow(hwnd);
		::UnregisterClass(wc.lpszClassName, wc.hInstance);

	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

	return 0;
}