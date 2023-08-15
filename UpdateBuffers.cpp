#include "stdafx.h"
#include "Thesis.h"

void Thesis::UpdateComputeConstantBuffer() {
	ComputeConstantBufferData bufferData{
		m_nbProbesPerLayer,
		m_nbProbeLayers,
		m_basePlaneSideLength,
		m_updateProbes,
		m_useOctagonalProbes,
		{m_lightPos[0], m_lightPos[1], m_lightPos[2]},
		{m_lightColor[0], m_lightColor[1], m_lightColor[2]},
		m_lightSpread,
		m_bounceAttenuation,
		m_useShadows,
		m_useDirectionalLight,
		m_nbCubes,
		{m_directionalLightAngle[0], m_directionalLightAngle[1], m_directionalLightAngle[2]},
		m_frameCount,
		m_nbSurfelsPerProbe,
		m_falloffRegulator,
		m_GIPreset == NoSurfels,
		m_multiBounceIntensity,
		m_useMultiBounce,
	};

	uint8_t* pData{ nullptr };
	ThrowIfFailed(m_computeConstantBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &bufferData, m_computeConstantBufferSize);
	m_computeConstantBuffer->Unmap(0, nullptr);
}

void Thesis::UpdateGlobalConstantBuffer() {
	GlobalConstantBufferData bufferData{
		{m_lightPos[0], m_lightPos[1], m_lightPos[2]},
		m_frameCount,
		{m_lightColor[0], m_lightColor[1], m_lightColor[2]},
		m_lightSpread,
		m_ambientTerm,
		m_nbProbesPerLayer,
		m_nbProbeLayers,
		m_basePlaneSideLength,
		{m_directionalLightAngle[0], m_directionalLightAngle[1], m_directionalLightAngle[2]},
		m_interpolateProbes,
		m_useAdvancedLighting,
		m_useProbeLighting,
		m_useShadows,
		m_useOctagonalProbes,
		m_useDirectionalLight,
		m_nbCubes + m_nbBoxes + m_nbIndexedPlanes + m_nbPlanes,
		m_nbProbeMarkers,
		m_probeBrightness,
	};

	uint8_t* pData{ nullptr };
	ThrowIfFailed(m_globalConstantBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &bufferData, sizeof(bufferData));
	m_globalConstantBuffer->Unmap(0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
// Creates and copies the viewmodel and perspective matrices for the camera.
//
void Thesis::UpdateCameraBuffer() {
	std::vector<XMMATRIX> matrices(4);

	// Initialize the view matrix. Ideally, this should be based on user interactions. The lookat and perspective 
	// matrices used for rasterization are defined to transform world-space vertices into a [0,1]x[0,1]x[0,1] camera 
	// space
	const glm::mat4& mat{ nv_helpers_dx12::CameraManip.getMatrix() };
	memcpy(&matrices[0].r->m128_f32[0], glm::value_ptr(mat), 16 * sizeof(float));

	float fovAngleY{ 45.0f * XM_PI / 180.0f };
	matrices[1] = XMMatrixPerspectiveFovRH(fovAngleY, m_aspectRatio, 0.1f, 1000.0f);

	// Raytracing has to do the contrary of rasterization: rays are defined in camera space, and are transformed into 
	// world space. To do this, we need to store the inverse matrices as well.
	XMVECTOR det;
	matrices[2] = XMMatrixInverse(&det, matrices[0]);
	matrices[3] = XMMatrixInverse(&det, matrices[1]);

	// Copy the matrix contents
	uint8_t* pData{ nullptr };
	ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, matrices.data(), m_cameraBufferSize);
	m_cameraBuffer->Unmap(0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
// Update settings to RayGen shader.
//
void Thesis::UpdateRenderSettingsBuffer() {
	RenderSettings settingsData{
		m_MSAAOn,
		m_renderProbeMarkers,
		m_renderLightMarkers,
		m_brightness,
	};

	uint8_t* pData{ nullptr };
	ThrowIfFailed(m_renderSettingsBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &settingsData, sizeof(settingsData));
	m_renderSettingsBuffer->Unmap(0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
// Copy the per-instance data into the buffer
// #DXR Extra: Refitting.
void Thesis::UpdateInstancePropertiesBuffer() {
	InstanceProperties* current{ nullptr };
	CD3DX12_RANGE readRange(0, 0);

	// We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_instanceProperties->Map(0, &readRange, reinterpret_cast<void**>(&current)));

	for (const auto& inst : m_instances) {
		current->objectToWorld = inst.second;

		// #DXR Extra: Simple lighting.
		XMMATRIX upper3x3 = inst.second;

		// Remove the translation and lower vector of the matrix
		upper3x3.r[0].m128_f32[3] = 0.f;
		upper3x3.r[1].m128_f32[3] = 0.f;
		upper3x3.r[2].m128_f32[3] = 0.f;
		upper3x3.r[3].m128_f32[0] = 0.f;
		upper3x3.r[3].m128_f32[1] = 0.f;
		upper3x3.r[3].m128_f32[2] = 0.f;
		upper3x3.r[3].m128_f32[3] = 1.f;


		XMVECTOR det;
		current->objectToWorldNormal = XMMatrixTranspose(XMMatrixInverse(&det, upper3x3));

		current++;
	}

	m_instanceProperties->Unmap(0, nullptr);
}

void Thesis::UpdateAABBBuffer() {
	std::vector<AABB> bufferData;

	for (uint8_t j{ 0 }; j < m_nbCubes; j++) {
		XMMATRIX transform = m_instances[j].second;

		if (!(XMMatrix4x4Compare(transform, m_prevTransform[j])))
		{
			std::vector<XMFLOAT4> newTransformedPoints{ TransformPoints(m_cubeVertices, transform) };
			std::vector<XMFLOAT4> oldTransformedPoints{ TransformPoints(m_cubeVertices, m_prevTransform[j]) };

			bufferData.push_back(PointsToAABB(newTransformedPoints.data()));
			bufferData.push_back(PointsToAABB(oldTransformedPoints.data()));
			m_prevTransform[j] = transform;
		}
		else {
			bufferData.push_back({ -D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX, -D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX,
				-D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX });
			bufferData.push_back({ -D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX, -D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX,
				-D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX });
		}
	}

	uint8_t* pData{ nullptr };
	ThrowIfFailed(m_AABBBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, bufferData.data(), m_AABBBufferSize);
	m_AABBBuffer->Unmap(0, nullptr);
}