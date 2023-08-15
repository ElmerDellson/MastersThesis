#include "stdafx.h"
#include "Thesis.h"

void Thesis::CreateCubeVB() {
	if (m_scene == Scene0) {
		Vertex cubeVertices[]{
			// Grey cubes.
			{{m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 0
			{{-m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 1
			{{-m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 2
			{{m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 3

			{{m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 4
			{{-m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 5
			{{-m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}}, // 6
			{{m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 7

			{{m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 8
			{{m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 9
			{{m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 10
			{{m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 11

			{{m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 12
			{{m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 13
			{{-m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 14
			{{-m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 15

			{{-m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 16
			{{-m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 17
			{{-m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 18
			{{-m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 19
		};

		memcpy(&m_cubeVertices, &cubeVertices, sizeof(cubeVertices));
	}
	else if (m_scene == Scene1) {
		Vertex cubeVertices[]{
			{{m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 0
			{{-m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 1
			{{-m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 2
			{{m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 3

			{{m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 4
			{{-m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 5
			{{-m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}}, // 6
			{{m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 7

			{{m_cubeScale, 0.f, m_cubeScale}, {0.f, 1.f, 0.f, 1.f}},	// 8
			{{m_cubeScale, 0.f, -m_cubeScale}, {0.f, 1.f, 0.f, 1.f}},	// 9
			{{m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {0.f, 1.f, 0.f, 1.f}},	 // 10
			{{m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {0.f, 1.f, 0.f, 1.f}},	 // 11

			{{m_cubeScale, 0.f, -m_cubeScale}, {0.f, 0.f, 1.f, 1.f}},	// 12
			{{m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {0.f, 0.f, 1.f, 1.f}},	// 13
			{{-m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {0.f, 0.f, 1.f, 1.f}},	 // 14
			{{-m_cubeScale, 0.f, -m_cubeScale}, {0.f, 0.f, 1.f, 1.f}},	 // 15

			{{-m_cubeScale, 0.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	// 16
			{{-m_cubeScale, m_cubeScale * 2.f, m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 17
			{{-m_cubeScale, m_cubeScale * 2.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}}, // 18
			{{-m_cubeScale, 0.f, -m_cubeScale}, {.7f, .7f, .7f, 1.f}},	 // 19
		};

		memcpy(&m_cubeVertices, &cubeVertices, sizeof(cubeVertices));
	}

	memcpy(&m_cubeVertices, &m_cubeVertices, sizeof(m_cubeVertices));
	m_nbVerticesPerCube = sizeof(m_cubeVertices) / sizeof(Vertex);
	const UINT vertexBufferSize{ sizeof(m_cubeVertices) };

	// Note: using upload heaps to transfer static data like vert buffers is not recommended. Every time the GPU needs 
	// it, the upload heap will be marshalled over. Please read up on Default Heap usage. An upload heap is used here 
	// for code simplicity and because there are very few verts to actually transfer.
	ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_cubeVertexBuffer)));

	// Copy the cube data to the vertex buffer.
	UINT8* pVertexDataBegin{ nullptr };
	CD3DX12_RANGE readRange(0, 0);		//< We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_cubeVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, m_cubeVertices, vertexBufferSize);
	m_cubeVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_cubeVertexBufferView.BufferLocation = m_cubeVertexBuffer->GetGPUVirtualAddress();
	m_cubeVertexBufferView.StrideInBytes = sizeof(Vertex);
	m_cubeVertexBufferView.SizeInBytes = vertexBufferSize;

	// Indices
	std::vector<UINT> indices{ 3, 1, 0,  3, 2, 1,  8, 11, 9,  8, 10, 11,
							   0, 1, 5,  5, 4, 0,  16, 18, 17,  16, 19, 18,
							   12, 14, 15,  12, 13, 14,  4, 5, 7,  7, 5, 6 };
	m_nbTrianglesPerCube = indices.size() / 3;

	const UINT indexBufferSize{ static_cast<UINT>(indices.size()) * sizeof(UINT) };
	CD3DX12_HEAP_PROPERTIES heapProperty{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD) };
	CD3DX12_RESOURCE_DESC bufferResource{ CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize) };

	ThrowIfFailed(m_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cubeIndexBuffer)));

	// Copy the cube data to the index buffer.
	UINT8* pIndexDataBegin{ nullptr };
	ThrowIfFailed(m_cubeIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
	m_cubeIndexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	m_cubeIndexBufferView.BufferLocation = m_cubeIndexBuffer->GetGPUVirtualAddress();
	m_cubeIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_cubeIndexBufferView.SizeInBytes = indexBufferSize;
}