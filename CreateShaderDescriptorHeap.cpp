#include "stdafx.h"
#include "Thesis.h"

// Create the buffer containing the raytracing result (always output in a UAV), and create the heap referencing the
// resources used by the raytracing, such as the acceleration structure.

//---------------------------------------------------------------------------------------------------------------------
// Create the main descriptor heap used by the shaders.
//
void Thesis::CreateShaderDescriptorHeap() {
	// Create descriptor heap, specifying type and number of entries!
	m_cbvSrvUavDescriptorHeap = nv_helpers_dx12::CreateDescriptorHeap(m_device.Get(), NbShaderDescriptors,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	NAME_D3D12_OBJECT(m_cbvSrvUavDescriptorHeap);
	m_cbvSrvUavDescriptorHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Get a handle to the descriptor heap memory on the CPU side, to be able to write the descriptors directly.
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{ m_cbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

	// Create the UAV for the output resource. Based on the root signature we created it is the first entry. The 
	// Create***View methods write the view information directly into srvHandle.
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(m_RTOutputResource.Get(), nullptr, &uavDesc, srvHandle);
	}

	// Create SRV for TLAS.
	{
		// Add the Top Level AS SRV right after the raytracing output buffer.
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.RaytracingAccelerationStructure.Location = m_topLevelASBuffers.pResult->GetGPUVirtualAddress();

		// Write the acceleration structure view in the heap
		m_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
	}

	// Create CBV for camera.
	{
		// Add the constant buffer for the camera after the TLAS.
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		// Describe and create a constant buffer view for the camera.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_cameraBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
	}

	// Create SRV for instance properties (I think).
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(m_instances.size());
		srvDesc.Buffer.StructureByteStride = sizeof(InstanceProperties);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// Write the per-instance properties buffer view in the heap.
		m_device->CreateShaderResourceView(m_instanceProperties.Get(), &srvDesc, srvHandle);
	}

	// Create CBV for the render settings.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		// Describe and create a constant buffer view for the settings.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = m_renderSettingsBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_renderSettingsBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
	}

	// Create UAV for compute shader probe output.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_nbProbeLayers * m_nbProbesPerLayer;
		uavDesc.Buffer.StructureByteStride = sizeof(Probe);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_device->CreateUnorderedAccessView(m_computeProbeBuffer.Get(), nullptr, &uavDesc, srvHandle);
		NAME_D3D12_OBJECT(m_computeProbeBuffer);
	}

	// Create CBV for the compute shader constant buffer.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = m_computeConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_computeConstantBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
	}

	// Create SRV for TLAS for the compute shader.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.RaytracingAccelerationStructure.Location = m_topLevelASBuffers.pResult->GetGPUVirtualAddress();

		m_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
	}

	// Create SRV for the cube VB for the compute shader.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_nbVerticesPerCube;
		srvDesc.Buffer.StructureByteStride = sizeof(Vertex);

		m_device->CreateShaderResourceView(m_cubeVertexBuffer.Get(), &srvDesc, srvHandle);
	}

	// Create SRV for the indexed plane VB for the compute shader.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_nbVerticesPerIndexedPlane;
		srvDesc.Buffer.StructureByteStride = sizeof(Vertex);

		m_device->CreateShaderResourceView(m_indexedPlanesVertexBuffer.Get(), &srvDesc, srvHandle);
	}

	// Create SRV for the cube index buffer for the compute shader.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_nbTrianglesPerCube * 3;
		srvDesc.Buffer.StructureByteStride = sizeof(int);

		m_device->CreateShaderResourceView(m_cubeIndexBuffer.Get(), &srvDesc, srvHandle);
	}

	// Create SRV for the indexed plane index buffer for the compute shader.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_nbTrianglesPerIndexedPlane * 3;
		srvDesc.Buffer.StructureByteStride = sizeof(int);

		m_device->CreateShaderResourceView(m_indexedPlanesIndexBuffer.Get(), &srvDesc, srvHandle);
	}

	// Create SRV for instance properties (I think) for the compute shader.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(m_instances.size());
		srvDesc.Buffer.StructureByteStride = sizeof(InstanceProperties);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// Write the per-instance properties buffer view in the heap.
		m_device->CreateShaderResourceView(m_instanceProperties.Get(), &srvDesc, srvHandle);
	}

	// Create UAV for compute shader surfel output.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_nbProbeLayers * m_nbProbesPerLayer * m_nbSurfelsPerProbe;
		uavDesc.Buffer.StructureByteStride = sizeof(Surfel);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_device->CreateUnorderedAccessView(m_surfelBuffer.Get(), nullptr, &uavDesc, srvHandle);
		NAME_D3D12_OBJECT(m_surfelBuffer);
	}

	// Create CBV for the AABBs sent to the compute shader to prompt probe updates.
	{
		srvHandle.ptr += m_cbvSrvUavDescriptorHeapSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = m_nbCubes * 2;
		srvDesc.Buffer.StructureByteStride = sizeof(AABB);

		m_device->CreateShaderResourceView(m_AABBBuffer.Get(), &srvDesc, srvHandle);
	}
}