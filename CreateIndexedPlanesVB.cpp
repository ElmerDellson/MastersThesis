#include "stdafx.h"
#include "Thesis.h"

void Thesis::CreateIndexedPlanesVB() {
	float redWallR{ 1.f }, redWallG{ 0.f }, redWallB{ 0.f };
	float blueWallR{ 0.f }, blueWallG{ 0.f }, blueWallB{ 1.f };
	float basePlaneR{ 0.7f }, basePlaneG{ 0.7f }, basePlaneB{ 0.7f };
	float greenWallR{ 0.f }, greenWallG{ 1.f }, greenWallB{ 0.f };
	float ceilingR{ 0.7f }, ceilingG{ 0.7f }, ceilingB{ 0.7f };
	float width = (m_distBetweenProbes * (sqrt(m_nbProbesPerLayer) - 2)) / 2 - 7.f;
	float top = m_distBetweenProbes * (m_nbProbeLayers - 2) + 3.f;
	float bottom = m_distBetweenProbes - 3.f;

	Vertex planeVertices[]{
		// Base plane.
		{{width, bottom, width}, {basePlaneR, basePlaneG, basePlaneB, 1.f}},	// 0
		{{-width, bottom, width}, {basePlaneR, basePlaneG, basePlaneB, 1.f}},	// 1
		{{-width, bottom, -width}, {basePlaneR, basePlaneG, basePlaneB, 1.f}},	// 2
		{{width, bottom, -width}, {basePlaneR, basePlaneG, basePlaneB, 1.f}},	// 3

		// Back wall.
		{{width, bottom, width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },	// 4
		{{width, top, width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },// 5
		{{-width, top, width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },// 6
		{{-width, bottom, width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },// 7

		// Right wall (blue).
		{{-width, bottom, width}, {blueWallR, blueWallG, blueWallB, 1.f} }, // 8
		{{-width, top, width}, {blueWallR, blueWallG, blueWallB, 1.f} }, // 9
		{{-width, top, -width}, {blueWallR, blueWallG, blueWallB, 1.f} }, // 10
		{{-width, bottom, -width}, {blueWallR, blueWallG, blueWallB, 1.f} }, // 11

		// Left wall (green).
		{{width, bottom, -width}, {greenWallR, greenWallG, greenWallB, 1.f} },// 12
		{{width, top, -width}, {greenWallR, greenWallG, greenWallB, 1.f} },// 13
		{{width, top, width}, {greenWallR, greenWallG, greenWallB, 1.f} },// 14
		{{width, bottom, width}, {greenWallR, greenWallG, greenWallB, 1.f} },	// 15

		// Ceiling
		{{width, top, width}, {ceilingR, ceilingG, ceilingB, 1.f}},	// 16
		{{-width, top, width}, {ceilingR, ceilingG, ceilingB, 1.f}},	// 17
		{{-width, top, -width}, {ceilingR, ceilingG, ceilingB, 1.f}},	// 18
		{{width, top, -width}, {ceilingR, ceilingG, ceilingB, 1.f}},	// 19

		{{width, bottom, -width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },	// 20
		{{width, top, -width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },// 21
		{{-width, top, -width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },// 22
		{{-width, bottom, -width}, {basePlaneR, basePlaneG, basePlaneB, 1.f} },// 23
	};

	m_nbVerticesPerIndexedPlane = sizeof(planeVertices) / sizeof(Vertex);
	const UINT vertexBufferSize{ sizeof(planeVertices) };

	ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_indexedPlanesVertexBuffer)));

	// Copy the plane data to the vertex buffer.
	UINT8* pVertexDataBegin{ nullptr };
	CD3DX12_RANGE readRange(0, 0);		//< We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_indexedPlanesVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, planeVertices, sizeof(planeVertices));
	m_indexedPlanesVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_indexedPlanesVertexBufferView.BufferLocation = m_indexedPlanesVertexBuffer->GetGPUVirtualAddress();
	m_indexedPlanesVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_indexedPlanesVertexBufferView.SizeInBytes = vertexBufferSize;

	// Indices
	std::vector<UINT> indices{};

	if (m_scene == Scene0) {
		indices = { 0, 1, 3,  1, 2, 3, // Base plane.
			4, 5, 6,  6, 7, 4, // Back wall.
			8, 9, 10,  10, 11, 8, // Right wall.
			12, 13, 15,  13, 14, 15, // Left wall.
			16, 18, 17,  19, 18, 16, // Ceiling.
			20, 22, 21,  20, 23, 22
		};
	}
	else if (m_scene == Scene1) {
		indices = { 0, 1, 3,  1, 2, 3, // Base plane.
					12, 13, 15,  13, 14, 15, // Left wall.
		};
	}

	m_nbTrianglesPerIndexedPlane = indices.size() / 3;
	const UINT indexBufferSize{ static_cast<UINT>(indices.size()) * sizeof(UINT) };

	CD3DX12_HEAP_PROPERTIES heapProperty{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	CD3DX12_RESOURCE_DESC bufferResource{ CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize) };

	ThrowIfFailed(m_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexedPlanesIndexBuffer)));

	// Copy the plane data to the index buffer.
	UINT8* pIndexDataBegin{ nullptr };
	ThrowIfFailed(m_indexedPlanesIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
	m_indexedPlanesIndexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	m_indexedPlanesIndexBufferView.BufferLocation = m_indexedPlanesIndexBuffer->GetGPUVirtualAddress();
	m_indexedPlanesIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexedPlanesIndexBufferView.SizeInBytes = indexBufferSize;
}