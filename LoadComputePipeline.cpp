#include "stdafx.h"
#include "Thesis.h"

void Thesis::LoadComputePipeline() {
	D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
	computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	ThrowIfFailed(m_device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_computeCommandQueue)));
	NAME_D3D12_OBJECT(m_computeCommandQueue);

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
		IID_PPV_ARGS(&m_computeCommandAllocator)));

	CreateComputePipelineRootSignature();

	CreateComputePipelineState();

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
		m_computeCommandAllocator.Get(), m_computePipelineState.Get(), IID_PPV_ARGS(&m_computeCommandList)));
	ThrowIfFailed(m_computeCommandList->Close());
	NAME_D3D12_OBJECT(m_computeCommandList);
}

void Thesis::CreateComputePipelineRootSignature() {
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion 
	// returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 ranges[5];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 1);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 2);
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
	ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

	CD3DX12_ROOT_PARAMETER1 computeRootParameters[ComputeRootParametersCount];
	computeRootParameters[SrvUavTable].InitAsDescriptorTable(_countof(ranges), ranges);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC computeRootSignatureDesc;
	computeRootSignatureDesc.Init_1_1(_countof(computeRootParameters), computeRootParameters);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&computeRootSignatureDesc, featureData.HighestVersion,
		&signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&m_computePipelineRootSignature)));
	NAME_D3D12_OBJECT(m_computePipelineRootSignature);
}

void Thesis::CreateComputePipelineState() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
	computePsoDesc.pRootSignature = m_computePipelineRootSignature.Get();
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)g_pComputeShader, ARRAYSIZE(g_pComputeShader));

	ThrowIfFailed(m_device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_computePipelineState)));
	NAME_D3D12_OBJECT(m_computePipelineState);
}