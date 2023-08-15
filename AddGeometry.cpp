#include "stdafx.h"
#include "Thesis.h"

void Thesis::AddGeometry() {
	// Build the bottom AS from the cube (previously triangle) vertex buffer.
	AccelerationStructureBuffers cubeBottomLevelBuffers = CreateBottomLevelAS(
		{ { m_cubeVertexBuffer.Get(), m_nbVerticesPerCube } },
		{ { m_cubeIndexBuffer.Get(), m_nbTrianglesPerCube * 3 } }
	);

	// Build BLAS for indexed planes.
	AccelerationStructureBuffers indexedPlaneBottomLevelBuffers = CreateBottomLevelAS(
		{ { m_indexedPlanesVertexBuffer.Get(), m_nbVerticesPerIndexedPlane } },
		{ { m_indexedPlanesIndexBuffer.Get(), m_nbTrianglesPerIndexedPlane * 3 } }
	);

	// Build BLAS for probe markers.
	AccelerationStructureBuffers probeMarkerBottomLevelBuffers = CreateBottomLevelAS(
		{ {m_probeMarkerVertexBuffer.Get(), m_nbVerticesPerProbeMarker} },
		{ {m_probeMarkerIndexBuffer.Get(), m_nbTrianglesPerProbeMarker * 3} }
	);

	// Adding geometry instances.

	// Cubes
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> cubes{};

	if (m_scene == Scene0) {
		std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> cubes0{
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(.5f, 1.f, .5f) * XMMatrixRotationY(.5f)
				* XMMatrixTranslation(60.f, 17.f, 60.f) },
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(.5f, .5f, .5f) * XMMatrixRotationY(-.5f)
				* XMMatrixTranslation(-60.f, 17.f, -60.f) },
		};

		cubes = cubes0;
	}
	else if (m_scene == Scene1) {
		std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> cubes1{
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(.6f, 3.f, 2.0f) * XMMatrixTranslation(120.f, 17.f, 15.f) },		// Door
			{ cubeBottomLevelBuffers.pResult, XMMatrixRotationY(glm::pi<float>()) * XMMatrixScaling(.6f, 3.f, 2.4f) * XMMatrixTranslation(120.f, 17.f, 122.f) },		// Far doorway
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(.6f, 3.f, 3.f) * XMMatrixTranslation(120.f, 17.f, -107.f) },		// Close doorway
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(.6f, 3.f, 7.31f) * XMMatrixTranslation(-178.f, 17.f, 0.f) },		// Right room wall
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(5.89f, 3.f, .6f) * XMMatrixTranslation(-42.5f, 17.f, 178.f) },	// Far room wall
			{ cubeBottomLevelBuffers.pResult, XMMatrixScaling(5.89f, 1.f, 7.31f) * XMMatrixTranslation(-42.5f, 166.f, 0.f) },	// Roof
		};

		cubes = cubes1;
	}

	m_nbCubes = cubes.size();

	// Indexed planes
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> indexedPlanes{
		{ indexedPlaneBottomLevelBuffers.pResult, XMMatrixIdentity() },
	};
	m_nbIndexedPlanes = indexedPlanes.size();

	// Probe markers 
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> probeMarkers{};
	/*{
		for (int y = 0; y < m_nbProbeLayers; y++) {
			for (int x = 0; x < sqrt(m_nbProbesPerLayer); x++) {
				for (int z = 0; z < sqrt(m_nbProbesPerLayer); z++) {
					float wPosX = x * m_distBetweenProbes - m_basePlaneSideLength / 2;
					float wPosY = y * m_distBetweenProbes;
					float wPosZ = z * m_distBetweenProbes - m_basePlaneSideLength / 2;

					probeMarkers.push_back({ probeMarkerBottomLevelBuffers.pResult, XMMatrixScaling(2.f, 2.f, 2.f)
						* XMMatrixTranslation(wPosX, wPosY, wPosZ) });
				}
			}
		}
	}*/
	m_nbProbeMarkers = probeMarkers.size();

	// Light markers
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> lightMarkers{
		{ probeMarkerBottomLevelBuffers.pResult, XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(0.f, 100.f, 0.f)},
	};
	m_nbLightMarkers = lightMarkers.size();

	m_instances.insert(m_instances.end(), cubes.begin(), cubes.end());
	m_instances.insert(m_instances.end(), indexedPlanes.begin(), indexedPlanes.end());
	m_instances.insert(m_instances.end(), probeMarkers.begin(), probeMarkers.end());
	m_instances.insert(m_instances.end(), lightMarkers.begin(), lightMarkers.end());

	// Initialize vector of transforms used to detect changes in geometry.
	for (uint8_t i{ 0 }; i < m_nbCubes; i++) {
		m_prevTransform.push_back(cubes[i].second);
	}
}