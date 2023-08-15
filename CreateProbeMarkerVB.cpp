#include "stdafx.h"
#include "Thesis.h"

void Thesis::CreateProbeMarkerVB() {
	float color = 1.f;
	Vertex vertices[]{
		{{1.f, 0.f, 1.f}, {color, color, color, 1.f}},	// 0
		{{-1.f, 0.f, 1.f}, {color, color, color, 1.f}},	// 1
		{{-1.f, 0.f, -1.f}, {color, color, color, 1.f}},	// 2
		{{1.f, 0.f, -1.f}, {color, color, color, 1.f}},	// 3
		{{0, 1.f, 0}, {color, color, color, 1.f}},	 // 4
		{{0, -1.f, 0}, {color, color, color, 1.f}},	 // 5
	};

	m_nbVerticesPerProbeMarker = sizeof(vertices) / sizeof(Vertex);
	const UINT vertexBufferSize{ sizeof(vertices) };

	ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_probeMarkerVertexBuffer)));

	UINT8* pVertexDataBegin{ nullptr };
	CD3DX12_RANGE readRange(0, 0);		//< We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_probeMarkerVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertices, sizeof(vertices));
	m_probeMarkerVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view. Is this needed?
	m_probeMarkerVertexBufferView.BufferLocation = m_probeMarkerVertexBuffer->GetGPUVirtualAddress();
	m_probeMarkerVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_probeMarkerVertexBufferView.SizeInBytes = vertexBufferSize;

	// Indices
	std::vector<UINT> indices{ 0, 4, 3,  1, 4, 0,  2, 4, 1,  3, 4, 2,
							   0, 3, 5,  1, 0, 5,  2, 1, 5,  3, 2, 5,
	};
	m_nbTrianglesPerProbeMarker = indices.size() / 3;
	const UINT indexBufferSize{ static_cast<UINT>(indices.size()) * sizeof(UINT) };

	CD3DX12_HEAP_PROPERTIES heapProperty{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	CD3DX12_RESOURCE_DESC bufferResource{ CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize) };

	ThrowIfFailed(m_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_probeMarkerIndexBuffer)));

	// Copy the plane data to the index buffer.
	UINT8* pIndexDataBegin{ nullptr };
	ThrowIfFailed(m_probeMarkerIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
	m_probeMarkerIndexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	m_probeMarkerIndexBufferView.BufferLocation = m_probeMarkerIndexBuffer->GetGPUVirtualAddress();
	m_probeMarkerIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_probeMarkerIndexBufferView.SizeInBytes = indexBufferSize;
}