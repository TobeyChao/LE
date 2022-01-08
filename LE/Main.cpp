#include "Demo.h"

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd
)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

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
		wc.hInstance = hInstance;
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
		RECT R = { 0, 0, 1280, 800 };
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
				// app->CalculateFrameStats();
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