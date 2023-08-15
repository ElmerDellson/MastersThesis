#include "stdafx.h"
#include "Thesis.h"

// Setup the acceleration structures (AS) for raytracing. When setting up 
// geometry, each bottom-level AS has its own transform matrix.

//---------------------------------------------------------------------------------------------------------------------
// Combine the BLAS and TLAS builds to construct the entire acceleration structure required to raytrace the scene.
//
void Thesis::CreateAccelerationStructures() {
	AddGeometry();
	CreateTopLevelAS();

	// Flush the command list and wait for it to finish.
	m_graphicsCommandList->Close();
	ID3D12CommandList* ppCommandLists[]{ m_graphicsCommandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(1, ppCommandLists);
	m_fenceValue++;
	m_graphicsCommandQueue->Signal(m_fence.Get(), m_fenceValue);
	m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);

	// Once the command list is finished executing, reset it to be reused for rendering.
	ThrowIfFailed(m_graphicsCommandList->Reset(m_graphicsCommandAllocator.Get(), m_graphicsPipelineState.Get()));
	ThrowIfFailed(m_graphicsCommandList->Close());
}

//-----------------------------------------------------------------------------
// Create the main acceleration structure that holds all instances of the scene.
// Similarly to the bottom-level AS generation, it is done in 3 steps: gathering
// the instances, computing the memory requirements for the AS, and building the
// AS itself
//
void Thesis::CreateTopLevelAS(bool updateOnly /*If true, the TLAS will only be refitted, not rebuilt*/) {

	if (!updateOnly) {
		// Gather all the instances into the builder helper.
		for (size_t i{ 0 }; i < m_instances.size(); i++) {
			UINT instanceInclusionMask{};
			if (i < m_instances.size() - m_nbIndexedPlanes - m_nbProbeMarkers - m_nbLightMarkers)
				instanceInclusionMask = 0x01; // Cubes.
			else if (i < m_instances.size() - m_nbProbeMarkers - m_nbLightMarkers)
				instanceInclusionMask = 0x02; // Indexed planes.
			else if (i < m_instances.size() - m_nbLightMarkers)
				instanceInclusionMask = 0x04; // Probe markers.
			else
				instanceInclusionMask = 0x08; // Light markers.

			m_topLevelASGenerator.AddInstance(m_instances[i].first.Get(), m_instances[i].second, static_cast<UINT>(i),
				static_cast<UINT>(2 * i) /*2 hit groups per instance, regular and shadow*/, instanceInclusionMask);
		}

		// As for the bottom-level AS, the building the AS requires some scratch space to store temporary data in addition 
		// to the actual AS. In the case of the top-level AS, the instance descriptors also need to be stored in GPU 
		// memory. This call outputs the memory requirements for each (scratch, results, instance descriptors) so that the 
		// application can allocate the corresponding memory.
		UINT64 scratchSize, resultSize, instanceDescsSize;

		m_topLevelASGenerator.ComputeASBufferSizes(m_device.Get(), true, &scratchSize, &resultSize, &instanceDescsSize);

		// Create the scratch and result buffers. Since the build is all done on GPU, those can be allocated on the 
		// default heap. 
		m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(m_device.Get(), scratchSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nv_helpers_dx12::kDefaultHeapProps);
		m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(m_device.Get(), resultSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nv_helpers_dx12::kDefaultHeapProps);

		// The buffer describing the instances: ID, shader binding information, matrices ... Those will be copied into the 
		// buffer by the helper through mapping, so the buffer has to be allocated on the upload heap. 
		m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(m_device.Get(), instanceDescsSize,
			D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	}

	// After all the buffers are allocated, or if only an update is required, we can build the acceleration structure. 
	// Note that in the case of the update we also pass the existing AS as the 'previous' AS, so that it can be 
	// refitted in place. 
	m_topLevelASGenerator.Generate(m_graphicsCommandList.Get(), m_topLevelASBuffers.pScratch.Get(),
		m_topLevelASBuffers.pResult.Get(), m_topLevelASBuffers.pInstanceDesc.Get(), updateOnly,
		m_topLevelASBuffers.pResult.Get());
}

//---------------------------------------------------------------------------------------------------------------------
// Create a bottom-level acceleration structure based on a list of vertex buffers in GPU memory along with their vertex 
// count. The build is then done in 3 steps: gathering the geometry, computing the sizes of the required buffers, and 
// building the actual AS
//
Thesis::AccelerationStructureBuffers Thesis::CreateBottomLevelAS(
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers) {
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;
	// Adding all vertex buffers and not transforming their position.

	for (size_t i{ 0 }; i < vVertexBuffers.size(); i++) {
		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second,
				sizeof(Vertex), vIndexBuffers[i].first.Get(), 0, vIndexBuffers[i].second, nullptr, 0, true);
		else
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second,
				sizeof(Vertex), 0, 0);
	}

	// The AS build requires some scratch space to store temporary information. 
	// The amount of scratch memory is dependent on the scene complexity. 
	UINT64 scratchSizeInBytes{ 0 };

	// The final AS also needs to be stored in addition to the existing vertex 
	// buffers. It size is also dependent on the scene complexity. 
	UINT64 resultSizeInBytes{ 0 };

	bottomLevelAS.ComputeASBufferSizes(m_device.Get(), false, &scratchSizeInBytes, &resultSizeInBytes);
	// Once the sizes are obtained, the application is responsible for allocating 
	// the necessary buffers. Since the entire generation will be done on the GPU, 
	// we can directly allocate those on the default heap 
	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(m_device.Get(), scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, nv_helpers_dx12::kDefaultHeapProps);
	buffers.pResult = nv_helpers_dx12::CreateBuffer(m_device.Get(), resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	// Build the acceleration structure. Note that this call integrates a barrier 
	// on the generated AS, so that it can be used to compute a top-level AS right 
	// after this method. 
	bottomLevelAS.Generate(m_graphicsCommandList.Get(), buffers.pScratch.Get(), buffers.pResult.Get(), false, nullptr);
	return buffers;
}