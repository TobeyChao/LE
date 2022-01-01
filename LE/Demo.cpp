#include "Demo.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "GeometryGenerator.h"

#include "../3rdParty/Assimp/include/assimp/Importer.hpp"
#include "../3rdParty/Assimp/include/assimp/PostProcess.h"
#include "../3rdParty/Assimp/include/assimp/Scene.h"

#pragma comment (lib, "../3rdParty/assimp/lib/assimp-vc142-mtd.lib")


bool show_demo_window = false;
bool show_another_window = false;
bool show_wireframe = false;
XMVECTORF32 clear_color = DirectX::Colors::DarkSlateGray;
ImFont* font;

Demo::~Demo()
{
	if (mD3D12Device != nullptr)
		FlushCommandQueue();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Demo::Initialize(HWND hwnd, int clientWidth, int clientHeight)
{
	SetDllDirectory(L"../3rdParty/Assimp/lib");

	auto lib = ::LoadLibrary(L"assimp-vc142-mtd.dll");

	D3D12App::Initialize(hwnd, clientWidth, clientHeight);

	mCameras["MainCamera"] = std::make_unique<Camera>();

#pragma region IMGUI

#pragma endregion

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(mD3D12Device.Get(), gNumFrameResources,
		DXGI_FORMAT_R8G8B8A8_UNORM, mSrvHeap->RawDH(),
		mSrvHeap->hCPU(0),
		mSrvHeap->hGPU(0));

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	font = io.Fonts->AddFontFromFileTTF("Fonts\\Zpix.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
	IM_ASSERT(font != NULL);

	mEyePos = { 0.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&mView, XMMatrixIdentity());
	XMStoreFloat4x4(&mProj, XMMatrixIdentity());

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// 加载图片资源
	LoadTextures();
	// 创建几何体
	BuildGeometry();
	BuildLandGeometry();

	// 创建材质
	BuildMaterials();
	// 创建着色器和输入布局
	BuildShadersAndInputLayout();
	// 根据着色器创建根描述符
	BuildRootSignature();

	// 创建渲染项
	BuildRenderItems();
	// 创建帧资源
	BuildFrameResources();

	// 创建描述符堆用于存储SRV
	BuildDescriptorHeaps();
	BuildShaderResourceViews();

	// 流水线状态
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	//mCommandAllocator->Reset();
	//mCommandList->Reset(mCommandAllocator.Get(), nullptr);

	//BuildComputeBuffers();
	//BuildComputeRootSignature();
	//BuildComputeShadersAndInputLayout();
	//BuildComputePSOs();

	//// Execute the initialization commands.
	//ThrowIfFailed(mCommandList->Close());
	//*cmdsLists = { mCommandList.Get() };
	//mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	//FlushCommandQueue();

	//DoComputeWork();
}

void Demo::OnResize()
{
	D3D12App::OnResize();
	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<float>(mClientWidth) / mClientHeight, 0.1f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Demo::Update()
{
	ProcessInput();
	UpdateCamera();
	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, FALSE, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs();
	UpdateMainPassCB();
	UpdateReflectedMainPassCB();
	UpdateMaterialCB();
}

void Demo::Draw()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Control Board");                          // Create a window called "Hello, world!" and append into it.
		ImGui::PushFont(font);
		ImGui::Text(u8"Display some 中文.");						// Display some text (you can use a format strings too)
		ImGui::PopFont();
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);
		ImGui::Checkbox("Wire Frame Mode", &show_wireframe);
		ImGui::Checkbox("MSAA", &mEnableMSAA);
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		//if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		//	counter++;
		//ImGui::SameLine();
		//ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), show_wireframe ? mPSOs["opaque_wireframe"].Get() : mPSOs["opaque_solid"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	if (mEnableMSAA)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			mMSAARenderTarget.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		mCommandList->ResourceBarrier(1, &barrier);

		auto rtvDescriptor = mMSAARtvHeap->hCPU(0);
		auto dsvDescriptor = mMSAADsvHeap->hCPU(0);

		mCommandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
		mCommandList->ClearRenderTargetView(rtvDescriptor, (float*)&clear_color, 0, nullptr);
		mCommandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	}
	else
	{
		// Indicate a state transition on the resource usage.
		mCommandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				CurrentBackBuffer(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Clear the back buffer and depth buffer.
		mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&clear_color, 0, nullptr);
		mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	}

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	UINT passCBByteSize = D3D12Util::CalcConstantBufferByteSize(sizeof(PassConstants));

	//int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
	//auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//passCbvHandle.Offset(passCbvIndex, mD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	//mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	// 渲染不透明物体
	mCommandList->SetGraphicsRootConstantBufferView(3, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	// 渲染曲面细分
	mCommandList->SetPipelineState(mPSOs["tess"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Tessellation]);

	// 渲染树
	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	// 渲染镜子
	mCommandList->OMSetStencilRef(1);
	mCommandList->SetPipelineState(mPSOs["markStencilMirrors"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Mirrors]);

	// 渲染镜子里的东西
	mCommandList->SetGraphicsRootConstantBufferView(3, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress() + 1 * passCBByteSize);
	mCommandList->SetPipelineState(mPSOs["drawStencilReflections"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Reflected]);
	mCommandList->SetGraphicsRootConstantBufferView(3, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
	mCommandList->OMSetStencilRef(0);

	// 渲染透明物体
	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	if (mEnableMSAA)
	{
		// Resolve the MSAA render target.
		auto backBuffer = CurrentBackBuffer();
		{
			D3D12_RESOURCE_BARRIER barriers[2] =
			{
				CD3DX12_RESOURCE_BARRIER::Transition(
					mMSAARenderTarget.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(
					backBuffer,
					D3D12_RESOURCE_STATE_PRESENT,
					D3D12_RESOURCE_STATE_RESOLVE_DEST)
			};

			mCommandList->ResourceBarrier(2, barriers);
		}

		mCommandList->ResolveSubresource(backBuffer, 0, mMSAARenderTarget.Get(), 0, mBackBufferFormat);

		// Set render target for UI which is typically rendered without MSAA.
		{
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer,
				D3D12_RESOURCE_STATE_RESOLVE_DEST,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			mCommandList->ResourceBarrier(1, &barrier);
		}
	}

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// 渲染UI
	ID3D12DescriptorHeap* descriptorHeapsSrv[] = { mSrvHeap->RawDH() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeapsSrv), descriptorHeapsSrv);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void Demo::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mYaw += dx;
		mPitch += dy;

		// Restrict the angle mPhi.
		//mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	//else if ((btnState & MK_LBUTTON) != 0)
	//{
	//	// Make each pixel correspond to 0.005 unit in the scene.
	//	float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
	//	float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

	//	// Update the camera radius based on input.
	//	mRadius += dx - dy;

	//	// Restrict the radius.
	//	mRadius = MathHelper::Clamp(mRadius, 3.0f, 150.0f);
	//}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Demo::ProcessInput()
{
	if (GetAsyncKeyState(0x57) & 0x8000)
		mCameras["MainCamera"]->MoveForward();
	if (GetAsyncKeyState(0x53) & 0x8000)
		mCameras["MainCamera"]->MoveBack();
	if (GetAsyncKeyState(0x41) & 0x8000)
		mCameras["MainCamera"]->MoveLeft();
	if (GetAsyncKeyState(0x44) & 0x8000)
		mCameras["MainCamera"]->MoveRight();
}

void Demo::UpdateCamera()
{
	// Convert Spherical to Cartesian coordinates.
	//mEyePos.x = mRadius * sinf(mPitch) * cosf(mYaw);
	//mEyePos.z = mRadius * sinf(mPitch) * sinf(mYaw);
	//mEyePos.y = mRadius * cosf(mPitch);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	mCameras["MainCamera"]->SetPitch(mPitch);
	mCameras["MainCamera"]->SetYaw(mYaw);
	mCameras["MainCamera"]->ComputeInfo();
	XMStoreFloat3(&mEyePos, mCameras["MainCamera"]->GetCameraPosition());
	const XMMATRIX& view = mCameras["MainCamera"]->GetViewMatrix()/*XMMatrixLookAtLH(pos, target, up)*/;
	XMStoreFloat4x4(&mView, view);
}

void Demo::UpdateObjectCBs()
{
	// Update the constant buffer with the latest worldViewProj matrix.
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void Demo::UpdateMainPassCB()
{
	// Update the pass buffer.
	auto currPassCB = mCurrFrameResource->PassCB.get();
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX view = XMLoadFloat4x4(&mView);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2{ (float)mClientWidth, (float)mClientHeight };
	mMainPassCB.InvRenderTargetSize = { 1.0f / mClientWidth, 1.0f / mClientHeight };
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = GameTimer::GetInstancePtr()->TotalTime();
	mMainPassCB.DeltaTime = GameTimer::GetInstancePtr()->DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };

	currPassCB->CopyData(0, mMainPassCB);
}

void Demo::UpdateReflectedMainPassCB()
{
	mReflectedPassCB = mMainPassCB;

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
	XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
	XMStoreFloat3(&mReflectedPassCB.Lights[0].Direction, reflectedLightDir);

	// Reflected pass stored in index 1
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectedPassCB);
}

void Demo::UpdateMaterialCB()
{
	auto materialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		auto mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants materialConstants;
			materialConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			materialConstants.FresnelR0 = mat->FresnelR0;
			materialConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&materialConstants.MatTransform, XMMatrixTranspose(matTransform));

			materialCB->CopyData(mat->MatCBIndex, materialConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void Demo::CalculateFrameStats()
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

void Demo::LoadTextures()
{
	auto gridTex = std::make_unique<Texture>();
	gridTex->Name = "tex_grid";
	gridTex->Filename = L"Textures/tex_grid.png";
	D3D12Util::LoadTexture(mD3D12Device.Get(), mCommandList.Get(), gridTex->Filename.c_str(),
		gridTex->Resource.GetAddressOf(), gridTex->UploadHeap.GetAddressOf());

	auto woodTex = std::make_unique<Texture>();
	woodTex->Name = "WoodCrate01";
	woodTex->Filename = L"Textures/WoodCrate01.dds";
	D3D12Util::LoadTexture(mD3D12Device.Get(), mCommandList.Get(), woodTex->Filename.c_str(),
		woodTex->Resource.GetAddressOf(), woodTex->UploadHeap.GetAddressOf());

	auto iceTex = std::make_unique<Texture>();
	iceTex->Name = "ice";
	iceTex->Filename = L"Textures/ice.dds";
	D3D12Util::LoadTexture(mD3D12Device.Get(), mCommandList.Get(), iceTex->Filename.c_str(),
		iceTex->Resource.GetAddressOf(), iceTex->UploadHeap.GetAddressOf());

	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"Textures/treeArray2.dds";
	D3D12Util::LoadTexture(mD3D12Device.Get(), mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource.GetAddressOf(), treeArrayTex->UploadHeap.GetAddressOf());

	mTextures[gridTex->Name] = std::move(gridTex);
	mTextures[woodTex->Name] = std::move(woodTex);
	mTextures[iceTex->Name] = std::move(iceTex);
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
}

void Demo::BuildMaterials()
{
	auto floor = std::make_unique<Material>();
	floor->Name = "floor";
	floor->MatCBIndex = 0;
	floor->DiffuseSrvHeapIndex = 0;
	floor->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.8f);
	floor->FresnelR0 = XMFLOAT3{ 0.01f, 0.01f, 0.01f };
	floor->Roughness = 0.8f;
	XMStoreFloat4x4(&(floor->MatTransform), XMMatrixScaling(2.0f, 2.0f, 1.f));

	auto wood = std::make_unique<Material>();
	wood->Name = "wood";
	wood->MatCBIndex = 1;
	wood->DiffuseSrvHeapIndex = 1;
	wood->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wood->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	wood->Roughness = 0.8f;
	XMStoreFloat4x4(&wood->MatTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

	auto icemirror = std::make_unique<Material>();
	icemirror->Name = "icemirror";
	icemirror->MatCBIndex = 2;
	icemirror->DiffuseSrvHeapIndex = 2;
	icemirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	icemirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	icemirror->Roughness = 0.5f;
	XMStoreFloat4x4(&icemirror->MatTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

	auto treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = 3;
	treeSprites->DiffuseSrvHeapIndex = 3;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;

	mMaterials["floor"] = std::move(floor);
	mMaterials["wood"] = std::move(wood);
	mMaterials["icemirror"] = std::move(icemirror);
	mMaterials["treeSprites"] = std::move(treeSprites);
}

void Demo::BuildRootSignature()
{
	// Default RootSignature
	{
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER slotRootParameter[4];
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsConstantBufferView(0);
		slotRootParameter[2].InitAsConstantBufferView(1);
		slotRootParameter[3].InitAsConstantBufferView(2);
		auto staticSamplers = GetStaticSamplers();

		CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(4, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> serializedRootSign = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSign.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}

		ThrowIfFailed(hr);

		ThrowIfFailed(mD3D12Device->CreateRootSignature(0,
			serializedRootSign->GetBufferPointer(),
			serializedRootSign->GetBufferSize(),
			IID_PPV_ARGS(mRootSignature.GetAddressOf())));
	}

	// Tessellation RootSignature
	{
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[4];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsConstantBufferView(0);
		slotRootParameter[2].InitAsConstantBufferView(1);
		slotRootParameter[3].InitAsConstantBufferView(2);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(mD3D12Device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mTessellationRootSignature.GetAddressOf())));
	}
}

void Demo::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = D3D12Util::CompileShader(L"Shaders\\Color.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = D3D12Util::CompileShader(L"Shaders\\Color.hlsl", nullptr, "PS", "ps_5_0");

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["treeSpriteVS"] = D3D12Util::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["treeSpriteGS"] = D3D12Util::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_0");
	mShaders["treeSpritePS"] = D3D12Util::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_0");

	//mShaders["tessVS"] = D3D12Util::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "VS", "vs_5_0");
	//mShaders["tessHS"] = D3D12Util::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "HS", "hs_5_0");
	//mShaders["tessDS"] = D3D12Util::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "DS", "ds_5_0");
	//mShaders["tessPS"] = D3D12Util::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "PS", "ps_5_0");

	mShaders["tessVS"] = D3D12Util::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["tessHS"] = D3D12Util::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["tessDS"] = D3D12Util::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["tessPS"] = D3D12Util::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "PS", "ps_5_0");

	mDefaultInputLayout.clear();
	mDefaultInputLayout.insert(mDefaultInputLayout.end(), std::begin(InputLayouts::inputLayoutPosTexNorCol), std::end(InputLayouts::inputLayoutPosTexNorCol));

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mTessellationInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void Demo::BuildGeometry()
{
	GeometryGenerator geoGen;
	// Grid
	{
		GeometryGenerator::MeshData model = geoGen.CreateGrid(64.0f, 64.0f, 4, 4);

		std::vector<PrimitiveTypes::PosTexNorColVertex> vertices(model.Vertices.size());

		for (size_t i = 0; i < model.Vertices.size(); ++i)
		{
			auto& p = model.Vertices[i];
			vertices[i].Position = { p.Position.x, p.Position.y/*GetHillsHeight(p.Position.x, p.Position.z)*/, p.Position.z };
			vertices[i].TexCoord = p.TexC;
			vertices[i].Normal = p.Normal/*GetHillsNormal(p.Position.x, p.Position.z)*/;
			vertices[i].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
		}

		std::vector<std::uint16_t> indices;
		indices.insert(indices.end(), std::begin(model.GetIndices16()), std::end(model.GetIndices16()));

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(PrimitiveTypes::PosTexNorColVertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "gridGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(PrimitiveTypes::PosTexNorColVertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["grid"] = submesh;
		mGeometries[geo->Name] = std::move(geo);
	}
	// Box
	{
		GeometryGenerator::MeshData model = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0);
		std::vector<PrimitiveTypes::PosTexNorColVertex> vertices(model.Vertices.size());

		for (size_t i = 0; i < model.Vertices.size(); ++i)
		{
			auto& p = model.Vertices[i];
			vertices[i].Position = { p.Position.x, p.Position.y, p.Position.z };
			vertices[i].TexCoord = p.TexC;
			vertices[i].Normal = p.Normal;
			vertices[i].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
		}

		std::vector<std::uint16_t> indices;
		indices.insert(indices.end(), std::begin(model.GetIndices16()), std::end(model.GetIndices16()));

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(PrimitiveTypes::PosTexNorColVertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "boxGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(PrimitiveTypes::PosTexNorColVertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submeshBox;
		submeshBox.IndexCount = (UINT)indices.size();
		submeshBox.StartIndexLocation = 0;
		submeshBox.BaseVertexLocation = 0;

		geo->DrawArgs["box"] = submeshBox;
		mGeometries[geo->Name] = std::move(geo);
	}
	// Mirror
	{
		GeometryGenerator::MeshData model = geoGen.CreateGrid(8.0f, 8.0f, 2, 2);

		std::vector<PrimitiveTypes::PosTexNorColVertex> vertices(model.Vertices.size());

		for (size_t i = 0; i < model.Vertices.size(); ++i)
		{
			auto& p = model.Vertices[i];
			vertices[i].Position = { p.Position.x, p.Position.y, p.Position.z };
			vertices[i].TexCoord = p.TexC;
			vertices[i].Normal = p.Normal;
			vertices[i].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
		}

		std::vector<std::uint16_t> indices;
		indices.insert(indices.end(), std::begin(model.GetIndices16()), std::end(model.GetIndices16()));

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(PrimitiveTypes::PosTexNorColVertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "mirrorGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(PrimitiveTypes::PosTexNorColVertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["mirror"] = submesh;
		mGeometries[geo->Name] = std::move(geo);
	}
	// Tree
	{
		struct TreeSpriteVertex
		{
			XMFLOAT3 Pos;
			XMFLOAT2 Size;
		};

		static const int treeCount = 16;
		std::array<TreeSpriteVertex, 16> vertices;
		for (UINT i = 0; i < treeCount; ++i)
		{
			float x = MathHelper::RandF(-16.0f, 16.0f);
			float z = MathHelper::RandF(-16.0f, 16.0f);
			float y = 1.5f;

			vertices[i].Pos = XMFLOAT3(x, y, z);
			vertices[i].Size = XMFLOAT2(4.0f, 4.0f);
		}

		std::array<std::uint16_t, 16> indices =
		{
			0, 1, 2, 3, 4, 5, 6, 7,
			8, 9, 10, 11, 12, 13, 14, 15
		};

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "treeSpritesGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(TreeSpriteVertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["points"] = submesh;

		mGeometries["treeSpritesGeo"] = std::move(geo);
	}
}

void Demo::BuildLandGeometry()
{
	std::array<XMFLOAT3, 16> vertices =
	{
		// Row 0
		XMFLOAT3(-10.0f, -10.0f, +15.0f),
		XMFLOAT3(-5.0f,  0.0f, +15.0f),
		XMFLOAT3(+5.0f,  0.0f, +15.0f),
		XMFLOAT3(+10.0f, 0.0f, +15.0f),

		// Row 1
		XMFLOAT3(-15.0f, 0.0f, +5.0f),
		XMFLOAT3(-5.0f,  0.0f, +5.0f),
		XMFLOAT3(+5.0f,  20.0f, +5.0f),
		XMFLOAT3(+15.0f, 0.0f, +5.0f),

		// Row 2
		XMFLOAT3(-15.0f, 0.0f, -5.0f),
		XMFLOAT3(-5.0f,  0.0f, -5.0f),
		XMFLOAT3(+5.0f,  0.0f, -5.0f),
		XMFLOAT3(+15.0f, 0.0f, -5.0f),

		// Row 3
		XMFLOAT3(-10.0f, 10.0f, -15.0f),
		XMFLOAT3(-5.0f,  0.0f, -15.0f),
		XMFLOAT3(+5.0f,  0.0f, -15.0f),
		XMFLOAT3(+25.0f, 10.0f, -15.0f)
	};

	std::array<std::int16_t, 16> indices =
	{
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15
	};

	const UINT vbByteSize = vertices.size() * sizeof(XMFLOAT3);
	const UINT ibByteSize = indices.size() * sizeof(std::int16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "quadpatchGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = D3D12Util::CreateDefaultBuffer(mD3D12Device.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(XMFLOAT3);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = 16;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["quadpatch"] = submesh;
	mGeometries[geo->Name] = std::move(geo);
}

void Demo::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(mD3D12Device.Get(),
			2, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void Demo::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 4;
	ThrowIfFailed(mD3D12Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(mSrvDescriptorHeap.GetAddressOf())));
}

void Demo::BuildShaderResourceViews()
{
	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto floorTex = mTextures["tex_grid"]->Resource;
	auto woodCrateTex = mTextures["WoodCrate01"]->Resource;
	auto iceTex = mTextures["ice"]->Resource;
	auto treeTex = mTextures["treeArrayTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = floorTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = floorTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	mD3D12Device->CreateShaderResourceView(floorTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	srvDesc.Format = woodCrateTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
	mD3D12Device->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	srvDesc.Format = iceTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = iceTex->GetDesc().MipLevels;
	mD3D12Device->CreateShaderResourceView(iceTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeTex->GetDesc().DepthOrArraySize;
	mD3D12Device->CreateShaderResourceView(treeTex.Get(), &srvDesc, hDescriptor);
}

void Demo::BuildRenderItems()
{
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 0;
	gridRitem->Mat = mMaterials["floor"].get();
	gridRitem->Geo = mGeometries["gridGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&(boxRitem->World), XMMatrixTranslation(0, 0.5f, -2));
	boxRitem->ObjCBIndex = 1;
	boxRitem->Mat = mMaterials["wood"].get();
	boxRitem->Geo = mGeometries["boxGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());

	auto reflectedBoxRitem = std::make_unique<RenderItem>();
	*reflectedBoxRitem = *boxRitem;
	reflectedBoxRitem->World = MathHelper::Identity4x4();

	// Update reflection world matrix.
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	XMStoreFloat4x4(&reflectedBoxRitem->World, XMLoadFloat4x4(&(boxRitem->World)) * R);
	reflectedBoxRitem->ObjCBIndex = 2;
	mRitemLayer[(int)RenderLayer::Reflected].push_back(reflectedBoxRitem.get());

	auto mirrorItem = std::make_unique<RenderItem>();
	mirrorItem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&(mirrorItem->World), XMMatrixRotationX(XMConvertToRadians(-90)));
	mirrorItem->ObjCBIndex = 3;
	mirrorItem->Mat = mMaterials["icemirror"].get();
	mirrorItem->Geo = mGeometries["mirrorGeo"].get();
	mirrorItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorItem->IndexCount = mirrorItem->Geo->DrawArgs["mirror"].IndexCount;
	mirrorItem->StartIndexLocation = mirrorItem->Geo->DrawArgs["mirror"].StartIndexLocation;
	mirrorItem->BaseVertexLocation = mirrorItem->Geo->DrawArgs["mirror"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Mirrors].push_back(mirrorItem.get());
	mRitemLayer[(int)RenderLayer::Transparent].push_back(mirrorItem.get());

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = 4;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	treeSpritesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());

	auto quadPatchRitem = std::make_unique<RenderItem>();
	quadPatchRitem->World = MathHelper::Identity4x4();
	quadPatchRitem->ObjCBIndex = 5;
	quadPatchRitem->Mat = mMaterials["floor"].get();
	quadPatchRitem->Geo = mGeometries["quadpatchGeo"].get();
	quadPatchRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
	quadPatchRitem->IndexCount = quadPatchRitem->Geo->DrawArgs["quadpatch"].IndexCount;
	quadPatchRitem->StartIndexLocation = quadPatchRitem->Geo->DrawArgs["quadpatch"].StartIndexLocation;
	quadPatchRitem->BaseVertexLocation = quadPatchRitem->Geo->DrawArgs["quadpatch"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Tessellation].push_back(quadPatchRitem.get());

	mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(boxRitem));
	mAllRitems.push_back(std::move(reflectedBoxRitem));
	mAllRitems.push_back(std::move(mirrorItem));
	mAllRitems.push_back(std::move(treeSpritesRitem));
	mAllRitems.push_back(std::move(quadPatchRitem));
}

void Demo::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mDefaultInputLayout.data(), (UINT)mDefaultInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = mEnableMSAA ? mSampleCount : 1;
	opaquePsoDesc.SampleDesc.Quality = mEnableMSAA ? (mMSAAQualityLevels - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_solid"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for opaque wireframe objects.
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

	//
	// PSO for mark stencil objects.
	//

	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	CD3DX12_DEPTH_STENCIL_DESC mirrorDSS;
	mirrorDSS.DepthEnable = true;
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	mirrorDSS.StencilEnable = true;
	mirrorDSS.StencilReadMask = 0xff;
	mirrorDSS.StencilWriteMask = 0xff;

	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorPsoDesc = opaquePsoDesc;
	markMirrorPsoDesc.BlendState = mirrorBlendState;
	markMirrorPsoDesc.DepthStencilState = mirrorDSS;
	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&markMirrorPsoDesc, IID_PPV_ARGS(&mPSOs["markStencilMirrors"])));

	//
	// PSO for draw relect objects.
	//

	CD3DX12_DEPTH_STENCIL_DESC reflectionDSS;
	reflectionDSS.DepthEnable = true;
	reflectionDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	reflectionDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	reflectionDSS.StencilEnable = true;
	reflectionDSS.StencilReadMask = 0xff;
	reflectionDSS.StencilWriteMask = 0xff;

	reflectionDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	reflectionDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	reflectionDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawStencilReflectionsPsoDesc = opaquePsoDesc;
	drawStencilReflectionsPsoDesc.DepthStencilState = reflectionDSS;
	drawStencilReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	drawStencilReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&drawStencilReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs["drawStencilReflections"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	if (mEnableMSAA)
	{
		treeSpritePsoDesc.BlendState.AlphaToCoverageEnable = true;
	}
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));

	//
	// PSO for tessellation sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC tessellationPsoDesc;
	ZeroMemory(&tessellationPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	tessellationPsoDesc.InputLayout = { mTessellationInputLayout.data(), (UINT)mTessellationInputLayout.size() };
	tessellationPsoDesc.pRootSignature = mTessellationRootSignature.Get();
	tessellationPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessVS"]->GetBufferPointer()),
		mShaders["tessVS"]->GetBufferSize()
	};
	tessellationPsoDesc.HS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessHS"]->GetBufferPointer()),
		mShaders["tessHS"]->GetBufferSize()
	};
	tessellationPsoDesc.DS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessDS"]->GetBufferPointer()),
		mShaders["tessDS"]->GetBufferSize()
	};
	tessellationPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["tessPS"]->GetBufferPointer()),
		mShaders["tessPS"]->GetBufferSize()
	};
	tessellationPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	tessellationPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	tessellationPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	tessellationPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	tessellationPsoDesc.SampleMask = UINT_MAX;
	tessellationPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	tessellationPsoDesc.NumRenderTargets = 1;
	tessellationPsoDesc.RTVFormats[0] = mBackBufferFormat;
	tessellationPsoDesc.SampleDesc.Count = mEnableMSAA ? mSampleCount : 1;
	tessellationPsoDesc.SampleDesc.Quality = mEnableMSAA ? (mMSAAQualityLevels - 1) : 0;
	tessellationPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(mD3D12Device->CreateGraphicsPipelineState(&tessellationPsoDesc, IID_PPV_ARGS(&mPSOs["tess"])));
}

void Demo::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = D3D12Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = D3D12Util::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();
	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		cmdList->SetGraphicsRootDescriptorTable(0, tex);

		D3D12_GPU_VIRTUAL_ADDRESS objAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(1, objAddress);

		D3D12_GPU_VIRTUAL_ADDRESS matAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(2, matAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Demo::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

void Demo::BuildComputeBuffers()
{
	std::vector<Data> dataA(NumDataElements);
	std::vector<Data> dataB(NumDataElements);
	for (int i = 0; i < NumDataElements; i++)
	{
		dataA[i].v1 = XMFLOAT3(i, i, i);
		dataA[i].v2 = XMFLOAT2(i, 0);

		dataB[i].v1 = XMFLOAT3(-i, i, 0.0f);
		dataB[i].v2 = XMFLOAT2(0, -i);
	}

	UINT64 byteSize = NumDataElements * sizeof(Data);

	mComputeInputBufferA = D3D12Util::CreateDefaultBuffer(
		mD3D12Device.Get(),
		mCommandList.Get(),
		dataA.data(),
		byteSize,
		mComputeInputUploadBufferA);

	mComputeInputBufferB = D3D12Util::CreateDefaultBuffer(
		mD3D12Device.Get(),
		mCommandList.Get(),
		dataB.data(),
		byteSize,
		mComputeInputUploadBufferB);

	mD3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mComputeOutputBuffer));

	mD3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mComputeReadBackBuffer));
}

void Demo::BuildComputeRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	slotRootParameter[0].InitAsShaderResourceView(0);
	slotRootParameter[1].InitAsShaderResourceView(1);
	slotRootParameter[2].InitAsUnorderedAccessView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	mD3D12Device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mComputeRootSignature.GetAddressOf()));
}

void Demo::BuildComputeShadersAndInputLayout()
{
	mShaders["vecAddCS"] = D3D12Util::CompileShader(L"Shaders\\VecAdd.hlsl", nullptr, "CS", "cs_5_0");
}

void Demo::BuildComputePSOs()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = mComputeRootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(mShaders["vecAddCS"]->GetBufferPointer()),
		mShaders["vecAddCS"]->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	mD3D12Device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&mPSOs["vecAdd"]));
}

void Demo::DoComputeWork()
{
	mCommandAllocator->Reset();

	mCommandList->Reset(mCommandAllocator.Get(), mPSOs["vecAdd"].Get());

	mCommandList->SetComputeRootSignature(mComputeRootSignature.Get());

	mCommandList->SetComputeRootShaderResourceView(0, mComputeInputBufferA->GetGPUVirtualAddress());
	mCommandList->SetComputeRootShaderResourceView(1, mComputeInputBufferB->GetGPUVirtualAddress());
	mCommandList->SetComputeRootUnorderedAccessView(2, mComputeOutputBuffer->GetGPUVirtualAddress());

	mCommandList->Dispatch(1, 1, 1);

	CD3DX12_RESOURCE_BARRIER barrier[1] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(
			mComputeOutputBuffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE)
	};

	mCommandList->ResourceBarrier(
		1,
		barrier
	);

	mCommandList->CopyResource(mComputeReadBackBuffer.Get(), mComputeOutputBuffer.Get());

	mCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			mComputeOutputBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_COMMON)
	);

	mCommandList->Close();

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait for the work to finish.
	FlushCommandQueue();

	Data* mappedData = nullptr;
	mComputeReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

	std::ofstream fout("results.txt");

	for (int i = 0; i < NumDataElements; ++i)
	{
		fout << "(" << mappedData[i].v1.x << ", " << mappedData[i].v1.y << ", " << mappedData[i].v1.z <<
			", " << mappedData[i].v2.x << ", " << mappedData[i].v2.y << ")" << std::endl;
	}

	mComputeReadBackBuffer->Unmap(0, nullptr);
}