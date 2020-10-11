#include "D3D12App.h"
#include "UploadBuffer.h"
#include "GameTimer.h"
#include "PrimitiveTypes.h"
#include "MeshGeometry.h"
#include <DirectXColors.h>

using namespace DirectX;

class Demo : public D3D12App
{
public:
	struct ObjectConstants
	{
		XMFLOAT4X4 WorldViewProj;
	};

	void Initialize(HWND hwnd, int clientWidth, int clientHeight) override
	{
		D3D12App::Initialize(hwnd, clientWidth, clientHeight);

		XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
		XMStoreFloat4x4(&mView, XMMatrixIdentity());
		XMStoreFloat4x4(&mProj, XMMatrixIdentity());

		ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

		// 自定义初始化
		BuildDescriptorHeaps();
		BuildConstantBuffers();
		BuildRootSignature();
		BuildShadersAndInputLayout();
		BuildBoxGeometry();
		BuildPSO();

		// Execute the initialization commands.
		ThrowIfFailed(mCommandList->Close());
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until initialization is complete.
		FlushCommandQueue();
	}

	void OnResize() override
	{
		D3D12App::OnResize();
		// The window resized, so update the aspect ratio and recompute the projection matrix.
		XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
		XMStoreFloat4x4(&mProj, P);
	}

	void Update() override
	{
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		// Build the view matrix.
		XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
		XMVECTOR target = XMVectorZero();
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
		XMStoreFloat4x4(&mView, view);

		XMMATRIX world = XMLoadFloat4x4(&mWorld);
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX worldViewProj = world * view * proj;

		// Update the constant buffer with the latest worldViewProj matrix.
		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
		mObjectCB->CopyData(0, objConstants);
	}

	void Draw() override;
	
	void CalculateFrameStats();
	
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();
private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;
};