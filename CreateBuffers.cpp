#include "stdafx.h"
#include "Thesis.h"

// Create AABB buffer.
void Thesis::CreateAABBBuffer() {
	m_AABBBufferSize = ROUND_UP(sizeof(AABB) * m_nbCubes * 2, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	m_AABBBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(), m_AABBBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	UpdateAABBBuffer();
}

// Allocate the buffer storing the raytracing output, with the same dimensions as the target image.
void Thesis::CreateRaytracingOutputBuffer() {
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats cannot be used with UAVs. For 
	// accuracy we should convert to sRGB ourselves in the shader
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = GetWidth();
	resDesc.Height = GetHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	ThrowIfFailed(m_device->CreateCommittedResource(&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_RTOutputResource)));
}

// Create constant buffer for the compute shader.
void Thesis::CreateComputeConstantBuffer() {
	int bufferSize{ sizeof(ComputeConstantBufferData) };
	m_computeConstantBufferSize = bufferSize + 256 - (bufferSize % 256);

	m_computeConstantBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(), m_computeConstantBufferSize,
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}

// Create constant buffers, with a color for each vertex of the triangle, for each triangle instance.
void Thesis::CreateGlobalConstantBuffer() {
	m_globalConstantBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(), sizeof(GlobalConstantBufferData),
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}

// Create buffers for probe output and readback from compute shader.
void Thesis::CreateOutputAndReadbackBuffers() {
	m_computeProbeBufferSize = sizeof(Probe) * m_nbProbeLayers * m_nbProbesPerLayer;

	// The output buffer is on a default heap, so only the GPU can access it.
	D3D12_RESOURCE_DESC probeBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(m_computeProbeBufferSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };

	ThrowIfFailed(m_device->CreateCommittedResource(&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&probeBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_computeProbeBuffer)));

	// The readback buffer is on a readback heap, so that the CPU can access it.
	D3D12_HEAP_PROPERTIES readbackHeapPropterties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK) };
	D3D12_RESOURCE_DESC readbackBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(m_computeProbeBufferSize) };

	ThrowIfFailed(m_device->CreateCommittedResource(&readbackHeapPropterties, D3D12_HEAP_FLAG_NONE, 
		&readbackBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_readbackBuffer)));
}

void Thesis::CreateQueryHeapAndResultBuffer() {
	D3D12_QUERY_HEAP_DESC queryHeapDesc{};
	queryHeapDesc.Count = 4 * 3; // Start and end for both compute and graphics, in a triple ring buffer.
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	m_device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&m_queryHeap));

	m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint64_t) * 4), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_queryResultBuffer));
}

//---------------------------------------------------------------------------------------------------------------------
// Buffer for sending settings to RayGen shader.
// 
void Thesis::CreateRenderSettingsBuffer() {
	// Create the constant buffer for all matrices.
	m_renderSettingsBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(), sizeof(RenderSettings),
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}

// Create buffer for surfels.
void Thesis::CreateSurfelBuffer() {
	m_surfelBufferSize = sizeof(Surfel) * m_nbSurfelsPerProbe * m_nbProbesPerLayer * m_nbProbeLayers;

	D3D12_RESOURCE_DESC surfelBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(m_surfelBufferSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };

	ThrowIfFailed(m_device->CreateCommittedResource(&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&surfelBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_surfelBuffer)));
}

//---------------------------------------------------------------------------------------------------------------------
// The camera buffer is a constant buffer that stores the transform matrices of the camera, for use by both the 
// rasterization and raytracing. This method allocates the buffer where the matrices will be copied, For the sake of 
// code clarity, it also creates a heap containing only this buffer, to use in the rasterization path.
//
void Thesis::CreateCameraBuffer() {
	// Create a buffer to store the modelview and perspective camera matrices.
	uint32_t nbMatrix = 4; // View, perspective, viewInv, perspectiveInv.

	m_cameraBufferSize = nbMatrix * sizeof(XMMATRIX);

	// Create the constant buffer for all matrices.
	m_cameraBuffer = nv_helpers_dx12::CreateBuffer(m_device.Get(), m_cameraBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}

//#DXR Extra: Refitting
//---------------------------------------------------------------------------------------------------------------------
// Allocate memory to hold per-instance information
// #DXR Extra: Refitting.
void Thesis::CreateInstancePropertiesBuffer() {
	uint32_t bufferSize = ROUND_UP(static_cast<uint32_t>(m_instances.size()) * sizeof(InstanceProperties),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	// Create the constant buffer for all matrices.
	m_instanceProperties = nv_helpers_dx12::CreateBuffer(m_device.Get(), bufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
}