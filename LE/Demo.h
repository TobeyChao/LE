#include "D3D12App.h"
#include "MathHelper.h"
#include "FrameResource.h"
#include "GameTimer.h"
#include "PrimitiveTypes.h"
#include "D3D12InputLayouts.h"
#include "Camera.h"
#include <DirectXColors.h>

using namespace DirectX;

enum class RenderLayer : int
{
	Opaque = 0,
	Mirrors,
	Reflected,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Shadow,
	Tessellation,
	Count
};

class Demo : public D3D12App
{
public:
	Demo();

	~Demo();

	void Initialize(HWND hwnd, int clientWidth, int clientHeight) override;

	void OnResize() override;

	void Update() override;

	void Draw() override;

	void OnMouseDown(WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mMainWnd);
	}
	void OnMouseUp(WPARAM btnState, int x, int y)
	{
		ReleaseCapture();
	}
	void OnMouseMove(WPARAM btnState, int x, int y) override;

	void ProcessInput();
	void UpdateCamera();
	void UpdateInstanceData();
	void UpdateObjectCBs();
	void UpdateMainPassCB();
	void UpdateReflectedMainPassCB();
	void UpdateMaterialCB();

	void CalculateFrameStats();

	void LoadTextures();
	void BuildMaterials();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildLandGeometry();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildDescriptorHeaps();
	void BuildShaderResourceViews();
	void BuildPSO();

	struct Data
	{
		XMFLOAT3 v1;
		XMFLOAT2 v2;
	};

	void BuildComputeBuffers();
	void DoComputeWork();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawRenderItemsNew(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	void PrepareUI();

private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	/*InputLayout*/
	std::vector<D3D12_INPUT_ELEMENT_DESC> mDefaultInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTessellationInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	//UINT mPassCbvOffset = 0;
	PassConstants mMainPassCB;
	PassConstants mReflectedPassCB;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

#pragma region Camera
	std::unordered_map<std::string, std::unique_ptr<Camera>> mCameras;

	float mYaw = 0;
	float mPitch = XMConvertToRadians(15);

	float mCamMoveSpeed = 20.f;

	POINT mLastMousePos;
#pragma endregion

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	int NumDataElements = 32;
	ComPtr<ID3D12RootSignature> mComputeRootSignature = nullptr;
	ComPtr<ID3D12Resource> mComputeInputBufferA = nullptr;
	ComPtr<ID3D12Resource> mComputeInputUploadBufferA = nullptr;
	ComPtr<ID3D12Resource> mComputeInputBufferB = nullptr;
	ComPtr<ID3D12Resource> mComputeInputUploadBufferB = nullptr;
	ComPtr<ID3D12Resource> mComputeOutputBuffer = nullptr;
	ComPtr<ID3D12Resource> mComputeReadBackBuffer = nullptr;

	ComPtr<ID3D12RootSignature> mTessellationRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mBillboardRootSignature = nullptr;
};