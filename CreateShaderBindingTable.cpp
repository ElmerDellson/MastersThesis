#include "stdafx.h"
#include "Thesis.h"

// Create the shader binding table and indicating which shaders are invoked for each instance in the AS.
//---------------------------------------------------------------------------------------------------------------------
// The Shader Binding Table (SBT) is the cornerstone of the raytracing setup: this is where the shader resources are 
// bound to the shaders in a way that can be interpreted by the raytracer on the GPU. In terms of layout, the SBT 
// contains the ray generation shader, the miss shaders, then the hit groups. Using the helper class, those can be
// specified in arbitrary order.
//
void Thesis::CreateShaderBindingTable() {

	// The SBT helper class collects calls to Add***Program. If called several times, the helper must be emptied before
	// re-adding shaders.
	m_sbtHelper.Reset();

	// The pointer to the beginning of the heap is the only parameter required by shaders without root parameters.
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle{ m_cbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart() };

	// The helper treats both root parameter pointers and heap pointers as void*, while DX12 uses the 
	// D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this struct is a UINT64, which then has to 
	// be reinterpreted as a pointer.
	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	// The ray generation only uses heap data.
	m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer /*Pointer to resource descriptor heap.*/ });
	// The miss and hit shaders do not access any external resources: instead, they communicate their results through
	// the ray payload.
	m_sbtHelper.AddMissProgram(L"Miss", {});
	// #DXR Extra: Another ray type.
	m_sbtHelper.AddMissProgram(L"ShadowMiss", {});

	// #DXR Extra: Per-instance data
	// We have 3 objects, each of which needs to access its own constant buffer as a root parameter in its primary 
	// hit shader. (The shadow hit only sets a boolean visibility in the payload, and does not require external data)
	for (int i{ 0 }; i < m_nbCubes; ++i) {
		m_sbtHelper.AddHitGroup(L"HitGroup", { (void*)(m_cubeVertexBuffer->GetGPUVirtualAddress()),
			(void*)(m_cubeIndexBuffer->GetGPUVirtualAddress()),
			(void*)(m_globalConstantBuffer->GetGPUVirtualAddress()),
			heapPointer,
			(void*)(m_computeProbeBuffer->GetGPUVirtualAddress()),
			});

		// #DXR Extra: Another ray type.
		m_sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	}

	for (int i{ 0 }; i < m_nbIndexedPlanes; ++i) {
		m_sbtHelper.AddHitGroup(L"HitGroup", { (void*)(m_indexedPlanesVertexBuffer->GetGPUVirtualAddress()),
			(void*)(m_indexedPlanesIndexBuffer->GetGPUVirtualAddress()),
			(void*)(m_globalConstantBuffer->GetGPUVirtualAddress()),
			heapPointer,
			(void*)(m_computeProbeBuffer->GetGPUVirtualAddress()),
			});

		m_sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	}

	for (int i{ 0 }; i < m_nbProbeMarkers; ++i) {
		m_sbtHelper.AddHitGroup(L"HitGroup", { (void*)(m_cubeVertexBuffer->GetGPUVirtualAddress()),
			(void*)(m_cubeIndexBuffer->GetGPUVirtualAddress()),
			(void*)(m_globalConstantBuffer->GetGPUVirtualAddress()),
			heapPointer,
			(void*)(m_computeProbeBuffer->GetGPUVirtualAddress()),
			});

		// #DXR Extra: Another ray type.
		m_sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	}

	uint32_t sbtSize{ m_sbtHelper.ComputeSBTSize() };

	// Create the SBT on the upload heap. This is required as the helper will use mapping to write the SBT contents.
	// After the SBT compilation it could be copied to the default heap for performance.
	m_sbtStorage = nv_helpers_dx12::CreateBuffer(m_device.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	if (!m_sbtStorage)
		throw std::logic_error("Could not allocate the shader binding table.");

	// Compile the SBT from the shader and parameters info.
	m_sbtHelper.Generate(m_sbtStorage.Get(), m_rtStateObjectProps.Get());
}