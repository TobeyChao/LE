#pragma once
#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "DDSTextureLoader12.h"
#include "WICTextureLoader12.h"

const int gNumFrameResources = 3;

#define MaxLights 16

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif // !ThrowIfFail

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif // !ReleaseCom

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString() const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

struct Material
{
	std::string Name;
	int MatCBIndex = -1;
	int DiffuseSrvHeapIndex = -1;
	int NumFramesDirty = gNumFrameResources;
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	DirectX::XMFLOAT4X4 MatTransform;
};

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	DirectX::XMFLOAT4X4 MatTransform;
};

struct Light
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float FalloffEnd = 10.0f;                           // point/spot light only
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float SpotPower = 64.0f;                            // spot light only
};

struct Texture
{
	std::string Name;
	std::wstring Filename;
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

class D3D12Util
{
public:
	static inline UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		// Constant buffers must be a multiple of the minimum hardware
		// allocation size (usually 256 bytes).  So round up to nearest
		// multiple of 256.  We do this by adding 255 and then masking off
		// the lower 2 bytes which store all bits < 256.
		// Example: Suppose byteSize = 300.
		// (300 + 255) & ~255
		// 555 & ~255
		// 0x022B & ~0x00ff
		// 0x022B & 0xff00
		// 0x0200
		// 512
		return (byteSize + 255) & 0xff00;
	}

	static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	static inline void LoadTexture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const wchar_t* filename,
		ID3D12Resource** ppResource,
		ID3D12Resource** ppUpload)
	{
		size_t len = wcsnlen_s(filename, 2048);
		if (len >= 4 && wcscmp(filename + len - 4, L".dds") == 0)
		{
			LoadDDSTexture(device, commandList, filename, ppResource, ppUpload);
		}
		else
		{
			LoadWICTexture(device, commandList, filename, ppResource, ppUpload);
		}
	}

	//static inline void CreateTextureSRV(
	//	ID3D12Device* device,
	//	ID3D12Resource* resource,
	//	DX::DescriptorHeap* descriptorHeap,
	//	UINT* descriptorHeapIndex,
	//	D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
	//	D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle,
	//	D3D12_SRV_DIMENSION srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D)
	//{
	//	// Describe and create an SRV.
	//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//	srvDesc.ViewDimension = srvDimension;
	//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	srvDesc.Format = resource->GetDesc().Format;
	//	srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
	//	srvDesc.Texture2D.MostDetailedMip = 0;
	//	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	//	*descriptorHeapIndex = descriptorHeap->AllocateDescriptor(cpuHandle, *descriptorHeapIndex);
	//	device->CreateShaderResourceView(resource, &srvDesc, *cpuHandle);
	//	*gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptorHeap->GetHeap()->GetGPUDescriptorHandleForHeapStart(),
	//		*descriptorHeapIndex, descriptorHeap->DescriptorSize());
	//};

	// Loads a DDS texture and issues upload on the commandlist. 
	// The caller is expected to execute the commandList.

private:

	static inline void LoadDDSTexture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const wchar_t* filename,
		ID3D12Resource** ppResource,
		ID3D12Resource** ppUpload,
		D3D12_SRV_DIMENSION srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D)
	{
		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		ThrowIfFailed(DirectX::LoadDDSTextureFromFile(device, filename, ppResource, ddsData, subresources));

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(*ppResource, 0, static_cast<UINT>(subresources.size()));

		// Create the GPU upload buffer.
		ThrowIfFailed(
			device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(ppUpload)));

		UpdateSubresources(commandList, *ppResource, *ppUpload, 0, 0, static_cast<UINT>(subresources.size()), subresources.data());
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*ppResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	}

	static inline void LoadWICTexture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const wchar_t* filename,
		ID3D12Resource** ppResource,
		ID3D12Resource** ppUpload)
	{
		std::unique_ptr<uint8_t[]> decodedData;
		D3D12_SUBRESOURCE_DATA subresource;
		ThrowIfFailed(DirectX::LoadWICTextureFromFile(device, filename, ppResource, decodedData, subresource));

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(*ppResource, 0, 1);

		// Create the GPU upload buffer.
		ThrowIfFailed(
			device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(ppUpload)));

		UpdateSubresources(commandList, *ppResource, *ppUpload, 0, 0, 1, &subresource);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*ppResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	}
};