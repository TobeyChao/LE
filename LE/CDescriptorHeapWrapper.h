#pragma once
#include "D3D12Util.h"

class CDescriptorHeapWrapper
{
public:
	CDescriptorHeapWrapper() { memset(this, 0, sizeof(*this)); }

	HRESULT Create(
		ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT NumDescriptors,
		bool bShaderVisible = false)
	{
		Desc.Type = Type;
		Desc.NumDescriptors = NumDescriptors;
		Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		Desc.NodeMask = 0;

		HRESULT hr = pDevice->CreateDescriptorHeap(&Desc,
			__uuidof(ID3D12DescriptorHeap),
			(void**)&pDH);
		if (FAILED(hr)) return hr;

		//hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
		//hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();

		HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
		return hr;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pDH->GetCPUDescriptorHandleForHeapStart(), index, HandleIncrementSize);
	}
	D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index)
	{
		assert(Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(pDH->GetGPUDescriptorHandleForHeapStart(), index, HandleIncrementSize);
	}
	UINT GetHandleIncrementSize()
	{
		return HandleIncrementSize;
	}
	ID3D12DescriptorHeap* RawDH()
	{
		return pDH.Get();
	}
private:
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDH;
	//D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
	//D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
	UINT HandleIncrementSize;
};