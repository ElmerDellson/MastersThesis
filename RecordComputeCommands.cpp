#include "stdafx.h"
#include "Thesis.h"

void Thesis::RecordComputeCommands() {
	m_fenceValue++;
	m_computeCommandQueue->Signal(m_fence.Get(), m_fenceValue);
	m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);

	//ThrowIfFailed(m_computeCommandAllocator.Reset());
	ThrowIfFailed(m_computeCommandList->Reset(m_computeCommandAllocator.Get(), m_computePipelineState.Get()));

	D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvUavDescriptorHandle =
		m_cbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	m_computeCommandList->SetComputeRootSignature(m_computePipelineRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvUavDescriptorHeap.Get() };
	m_computeCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_computeCommandList->SetComputeRootDescriptorTable(SrvUavTable,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvSrvUavDescriptorHandle, ComputeShaderOutputIdx,
			m_cbvSrvUavDescriptorHeapSize));

	// Transition probe buffer from copy destination to unordered access, to allow the compute shader to write to it.
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_computeProbeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_computeCommandList->ResourceBarrier(1, &barrier);

	// Start recording time.
	m_computeCommandList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex + 0);

	m_computeCommandList->Dispatch((int)sqrt(m_nbProbesPerLayer), m_nbProbeLayers, (int)sqrt(m_nbProbesPerLayer));

	m_computeCommandList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex + 1);

	int8_t queryIndexToResolve = queryIndex - 4;
	if (queryIndexToResolve < 0) queryIndexToResolve = 8;
	m_computeCommandList->ResolveQueryData(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndexToResolve, 2, m_queryResultBuffer.Get(), 0);

	// Transition probe buffer from unorered accces to copy source, to allow copy to readback buffer.
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_computeProbeBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_computeCommandList->ResourceBarrier(1, &barrier);

	// Copy probe buffer to readback buffer for debugging.
	m_computeCommandList->CopyResource(m_readbackBuffer.Get(), m_computeProbeBuffer.Get());

	// Transition probe buffer from copy source to unordered access, to allow the graphics pipeline to read from it(?).
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_computeProbeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_computeCommandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(m_computeCommandList->Close());
}