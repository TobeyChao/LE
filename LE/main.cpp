#include "D3D12App.h"
#include "GameTimer.h"
#include <DirectXColors.h>

using namespace DirectX;

class Demo : public D3D12App
{
public:
	void OnResize() override
	{
		D3D12App::OnResize();
	}

	void Update() override
	{

	}

	void Draw() override
	{
		// Rendering
		ThrowIfFailed(mCommandAllocator->Reset());

		mCommandList->Reset(mCommandAllocator.Get(), nullptr);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		ThrowIfFailed(mCommandList->Close());

		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		//m_swap_chain->Present(1, 0); // Present with vsync
		mSwapChain->Present(0, 0); // Present without vsync
		mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;

		FlushCommandQueue();
	}

	void calculate_frame_stats()
	{
		// Code computes the average frames per second, and also the 
		// average time it takes to render one frame.  These stats 
		// are appended to the window caption bar.

		static int frameCnt = 0;
		static float timeElapsed = 0.0f;

		frameCnt++;

		// Compute averages over one second period.
		if ((GameTimer::GetInstancePtr()->TotalTime() - timeElapsed) >= 1.0f)
		{
			float fps = (float)frameCnt; // fps = frameCnt / 1
			float mspf = 1000.0f / fps;

			std::wstring fpsStr = std::to_wstring(fps);
			std::wstring mspfStr = std::to_wstring(mspf);

			std::wstring windowText = L"D3D12App fps: " + fpsStr + L"   mspf: " + mspfStr;

			SetWindowText(mMainWnd, windowText.c_str());

			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}
	}

private:

};

// Main code
int main(int, char**)
{
	// Initialize Direct3D
	try
	{
		GameTimer* timer = new GameTimer();
		Demo* app = new Demo();

		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = Demo::MainWndProc;
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

		HWND hwnd = CreateWindow(wc.lpszClassName, L"D3D12App",
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, wc.hInstance, 0);

		// Create application window
		//WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, demo::main_wnd_proc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
		//::RegisterClassEx(&wc);
		//HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Dear ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
		app->Initialize(hwnd, 800, 600);

		// Show the window
		::ShowWindow(hwnd, SW_SHOW);
		::UpdateWindow(hwnd);

		// Main loop
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		GameTimer::GetInstancePtr()->Reset();
		while (msg.message != WM_QUIT)
		{
			if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				continue;
			}

			GameTimer::GetInstancePtr()->Tick();

			if (!app->IsPaused())
			{
				app->calculate_frame_stats();
				app->Update();
				app->Draw();
			}
			else
			{
				Sleep(100);
			}
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