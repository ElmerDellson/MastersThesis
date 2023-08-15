#include "stdafx.h"
#include "Thesis.h"

void Thesis::RecordGraphicsCommands() {
	// Command list allocators can only be reset when the associated command lists have finished execution on the GPU; 
	// apps should use fences to determine GPU execution progress. However, when ExecuteCommandList() is called on a 
	// particular command list, that command list can then be reset at any time and must be before re-recording.
	m_fenceValue++;
	m_graphicsCommandQueue->Signal(m_fence.Get(), m_fenceValue);
	m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);

	ThrowIfFailed(m_graphicsCommandAllocator->Reset());
	ThrowIfFailed(m_graphicsCommandList->Reset(m_graphicsCommandAllocator.Get(), m_graphicsPipelineState.Get()));

	// Set necessary state.
	m_graphicsCommandList->SetGraphicsRootSignature(m_graphicsPipelineRootSignature.Get());
	m_graphicsCommandList->RSSetViewports(1, &m_viewport);
	m_graphicsCommandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex,
		m_rtvDescriptorHeapSize);

	RecordRaytracingCommands(rtvHandle);

	// Indicate that the back buffer will now be used to present.
	m_graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_graphicsCommandList->Close());
}

void Thesis::RecordRaytracingCommands(CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle)
{
	// #DXR Extra: Refitting.
	// Refit the top-level acceleration structure to account for the new transform matrix of the geometry. Note that
	// the build contains a barrier, hence we can do the rendering in the same command list.
	CreateTopLevelAS(true);

	CD3DX12_RESOURCE_BARRIER transition;

	PrepareRaytracing(rtvHandle, transition);
	DoRaytracing();
	CopyRTOutputToRenderTarget(transition);
}

void Thesis::PrepareRaytracing(CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
	CD3DX12_RESOURCE_BARRIER& transition) {
	const float clearColor[]{ 1.0f, 0.8f, 0.4f, 1.0f };
	m_graphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Bind the descriptor heap giving access to the TLAS, as well as the raytracing output.
	std::vector<ID3D12DescriptorHeap*> heaps{ m_cbvSrvUavDescriptorHeap.Get() };
	m_graphicsCommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

	// On the last frame, the raytracing output was used as a copy source, to copy its contents into the render
	// target. Now we need to transition it to a UAV so that the shaders can write in it.
	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_RTOutputResource.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_graphicsCommandList->ResourceBarrier(1, &transition);
}

void Thesis::DoRaytracing() {
	// Setup the raytracing task.
	D3D12_DISPATCH_RAYS_DESC desc{};

	// The layout of the SBT is as follows: ray generation shader, miss shaders, hit groups. As described in the 
	// CreateShaderBindingTable method, all SBT entries of a given type have the same size to allow a fixed stride.
	// The ray generation shaders are always at the beginning of the SBT.
	uint32_t rayGenerationSectionSizeInBytes{ m_sbtHelper.GetRayGenSectionSize() };
	desc.RayGenerationShaderRecord.StartAddress = m_sbtStorage->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

	// The miss shaders are in the second SBT section, right after the ray generation shader. 
	// "We have one miss shader for the camera rays and one for the shadow rays, so this section has a size of 
	// 2*m_sbtEntrySize." We also indicate the stride between the two miss shaders, which is the size of an SBT entry.
	uint32_t missSectionSizeInBytes{ m_sbtHelper.GetMissSectionSize() };
	desc.MissShaderTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
	desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
	desc.MissShaderTable.StrideInBytes = m_sbtHelper.GetMissEntrySize();

	// The hit groups section starts after the miss shaders. In this sample we have 1 hit group for the triangle.
	uint32_t hitGroupsSectionSize{ m_sbtHelper.GetHitGroupSectionSize() };
	desc.HitGroupTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes +
		missSectionSizeInBytes;
	desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
	desc.HitGroupTable.StrideInBytes = m_sbtHelper.GetHitGroupEntrySize();

	// Dimensions of the image to render, identical to a kernel launch dimension.
	desc.Width = GetWidth();
	desc.Height = GetHeight();
	desc.Depth = 1;

	// Bind the raytracing pipeline.
	m_graphicsCommandList->SetPipelineState1(m_rtStateObject.Get());

	m_graphicsCommandList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex + 2);

	// Dispatch the rays and write to the raytracing output.
	m_graphicsCommandList->DispatchRays(&desc);

	m_graphicsCommandList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex + 3);

	int8_t queryIndexToResolve = queryIndex - 4;
	if (queryIndexToResolve < 0) queryIndexToResolve = 8;
	m_graphicsCommandList->ResolveQueryData(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndexToResolve + 2, 2, 
		m_queryResultBuffer.Get(), 2 * sizeof(uint64_t));

	// Transition probe buffer from unordered access to copy destination.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_computeProbeBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
	m_graphicsCommandList->ResourceBarrier(1, &transition);
}