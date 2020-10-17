#include "D3D12App.h"
#include "MathHelper.h"
#include "FrameResource.h"
#include "GameTimer.h"
#include "PrimitiveTypes.h"
#include "MeshGeometry.h"
#include <DirectXColors.h>

using namespace DirectX;

const int gNumFrameResources = 3;

struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

class Demo : public D3D12App
{
public:
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
	
	void CalculateFrameStats();
	
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildPSO();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	UINT mPassCbvOffset = 0;
	PassConstants mMainPassCB;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	XMFLOAT3 mEyePos;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta = XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};