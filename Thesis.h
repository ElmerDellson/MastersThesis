//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"
#include "DXRHelper.h"
#include <stdexcept>
#include <dxcapi.h>
#include <vector>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <iomanip>
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "nv_helpers_dx12/RootSignatureGenerator.h"
#include "manipulator.h"
#include "obj\x64\Debug\CompiledShaders\ComputeShader.hlsl.h"
#include "Windowsx.h"
#include "glm/gtc/type_ptr.hpp"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class Thesis : public DXSample
{
public:
	Thesis(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	void HandleAnimation(bool runBenchmark);
	virtual void OnRender();
	virtual void OnDestroy();

private:
	static const UINT FrameCount{ 2 };
	uint8_t queryIndex{ 0 };

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device5> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	UINT m_rtvDescriptorHeapSize;

	// Graphics pipline objects.
	ComPtr<ID3D12CommandAllocator> m_graphicsCommandAllocator;
	ComPtr<ID3D12CommandQueue> m_graphicsCommandQueue;
	ComPtr<ID3D12RootSignature> m_graphicsPipelineRootSignature;
	ComPtr<ID3D12PipelineState> m_graphicsPipelineState;
	ComPtr<ID3D12GraphicsCommandList4> m_graphicsCommandList;

	// Compute pipeline objects.
	ComPtr<ID3D12CommandAllocator> m_computeCommandAllocator;
	ComPtr<ID3D12CommandQueue> m_computeCommandQueue;
	ComPtr<ID3D12RootSignature> m_computePipelineRootSignature;
	ComPtr<ID3D12PipelineState> m_computePipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_computeCommandList;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	void LoadGraphicsPipeline(ComPtr<IDXGIFactory4> factory);
	void LoadComputePipeline();
	void CreateSwapChain(Microsoft::WRL::ComPtr<IDXGIFactory4>& factory);
	void CreateDevice(Microsoft::WRL::ComPtr<IDXGIFactory4>& factory);
	void CreateFrameResources();
	void CreateRTVDescriptorHeap();
	void EnableDebugLayer(UINT& dxgiFactoryFlags);
	void LoadAssets();
	void CreateSyncObjectsAndWait();
	void CreateGraphicsPipelineState();
	void CreateGraphicsPipelineRootSignature();
	void CreateComputePipelineState();
	void CreateComputePipelineRootSignature();

	void RecordComputeCommands();
	void RecordGraphicsCommands();
	void RecordRaytracingCommands(CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle);
	void PrepareRaytracing(CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, CD3DX12_RESOURCE_BARRIER& transition);
	void DoRaytracing();
	void CopyRTOutputToRenderTarget(CD3DX12_RESOURCE_BARRIER& transition);
	void WaitForPreviousFrame();
	void CheckRaytracingSupport();
	void OnKeyUp(UINT8 key);
	void OnKeyDown(UINT8 key);
	void ExportTimings();
	void HandleReadbackBuffers();

	struct AccelerationStructureBuffers {
		ComPtr<ID3D12Resource> pScratch; //< Scratch memory for AS builder
		ComPtr<ID3D12Resource> pResult; //< Where the AS is stored
		ComPtr<ID3D12Resource> pInstanceDesc; //< Holds the matrices of the instances (for TLAS, I think)
	};

	nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructureBuffers m_topLevelASBuffers;
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;

	/// Create the acceleration structure of an instance
	///
	/// \param vVertexBuffers : pair of buffer and vertex count
	/// \return AccelerationStructureBuffers for TLAS
	AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>,
		uint32_t>> vVertexBuffers, std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers = {});

	/// Create the main acceleration structure that holds
	/// all instances of the scene
	/// \param instances : pair of BLAS and transform
	/// #DXR Extra: Refitting
	/// \param updateOnly: if true, perform a refit instead of a full build.
	void CreateTopLevelAS(bool updateOnly = false);

	/// Create all acceleration structures, bottom and top
	void AddGeometry();
	void CreateAccelerationStructures();

	// Shader root signatures
	ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	ComPtr<ID3D12RootSignature> CreateMissSignature();
	ComPtr<ID3D12RootSignature> CreateHitSignature();
	ComPtr<ID3D12RootSignature> CreateProbePlaneHitSignature();

	void CreateRaytracingPipeline();

	ComPtr<IDxcBlob> m_rayGenLibrary;
	ComPtr<IDxcBlob> m_hitLibrary;
	ComPtr<IDxcBlob> m_missLibrary;
	ComPtr<ID3D12RootSignature> m_rayGenSignature;
	ComPtr<ID3D12RootSignature> m_hitSignature;
	ComPtr<ID3D12RootSignature> m_missSignature;

	// Raytracing pipeline state
	ComPtr<ID3D12StateObject> m_rtStateObject;

	// Raytracing pipeline state properties, retaining the shader identifiers to use in the shader binding table.
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

	void CreateRaytracingOutputBuffer();
	void CreateShaderDescriptorHeap();
	ComPtr<ID3D12Resource> m_RTOutputResource;
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavDescriptorHeap;
	UINT m_cbvSrvUavDescriptorHeapSize;

	void CreateShaderBindingTable();
	nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
	ComPtr<ID3D12Resource> m_sbtStorage;

	// Cubes
	ComPtr<ID3D12Resource> m_cubeVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_cubeVertexBufferView;
	ComPtr<ID3D12Resource> m_cubeIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_cubeIndexBufferView;
	void CreateCubeVB();
	int m_nbVerticesPerCube{ 0 };
	int m_nbTrianglesPerCube{ 0 };
	Vertex m_cubeVertices[20];

	// Indexed planes
	ComPtr<ID3D12Resource> m_indexedPlanesVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_indexedPlanesVertexBufferView;
	ComPtr<ID3D12Resource> m_indexedPlanesIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexedPlanesIndexBufferView;
	void CreateIndexedPlanesVB();
	int m_nbVerticesPerIndexedPlane{ 0 };
	int m_nbTrianglesPerIndexedPlane{ 0 };

	// Probe markers
	ComPtr<ID3D12Resource> m_probeMarkerVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_probeMarkerVertexBufferView;
	ComPtr<ID3D12Resource> m_probeMarkerIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_probeMarkerIndexBufferView;
	void CreateProbeMarkerVB();
	int m_nbVerticesPerProbeMarker{ 0 };
	int m_nbTrianglesPerProbeMarker{ 0 };

	// #DXR Extra: Perspective camera
	void CreateCameraBuffer();
	void UpdateCameraBuffer();
	ComPtr<ID3D12Resource> m_cameraBuffer;
	uint32_t m_cameraBufferSize{ 0 };

	// #DXR Extra: Perspective Camera++
	void OnButtonDown(UINT32 lParam);
	void OnMouseMove(UINT8 wParam, UINT32 lParam);

	// #DXR Extra: Per-instance data
	void CreateGlobalConstantBuffer();
	void UpdateGlobalConstantBuffer();
	ComPtr<ID3D12Resource> m_globalConstantBuffer;

	void CreateComputeConstantBuffer();
	void UpdateComputeConstantBuffer();
	ComPtr<ID3D12Resource> m_computeConstantBuffer;
	uint32_t m_computeConstantBufferSize{ 0 };

	// #DXR Extra: Another ray type.
	ComPtr<IDxcBlob> m_shadowLibrary;
	ComPtr<ID3D12RootSignature> m_shadowSignature;

	// Probes
	ComPtr<IDxcBlob> m_probeLibrary;
	ComPtr<ID3D12RootSignature> m_probePlaneSignature;

	// #DXR Extra: Refitting
	uint32_t m_time{ 0 };
	uint32_t m_frameCount{ 0 };

	// Per-instance properties
	struct InstanceProperties {
		XMMATRIX objectToWorld;

		// #DXR Extra: Simple lighting 
		XMMATRIX objectToWorldNormal;
	};

	ComPtr<ID3D12Resource> m_instanceProperties;
	void CreateInstancePropertiesBuffer();
	void UpdateInstancePropertiesBuffer();

	// Settings buffer.
	void CreateRenderSettingsBuffer();
	void UpdateRenderSettingsBuffer();
	ComPtr<ID3D12Resource> m_renderSettingsBuffer;
	uint32_t m_renderSettingsBufferSize{ 0 };

	// Probe buffer and readback buffer for compute shader.
	void CreateOutputAndReadbackBuffers();
	ComPtr<ID3D12Resource> m_computeProbeBuffer;
	ComPtr<ID3D12Resource> m_readbackBuffer;
	UINT64 m_computeProbeBufferSize{ 0 };
	uint32_t m_readbackBufferSize{ 0 };

	// Surfel buffer.
	void CreateSurfelBuffer();
	ComPtr<ID3D12Resource> m_surfelBuffer;
	uint32_t m_surfelBufferSize{ 0 };

	// AABB buffer for prompting probe updates.
	void CreateAABBBuffer();
	void UpdateAABBBuffer();
	ComPtr<ID3D12Resource> m_AABBBuffer;
	uint32_t m_AABBBufferSize{ 0 };
	std::vector<XMMATRIX> m_prevTransform;

	// Create query heap and query readback buffer.
	void CreateQueryHeapAndResultBuffer();
	ComPtr<ID3D12QueryHeap> m_queryHeap;
	ComPtr<ID3D12Resource> m_queryResultBuffer;
	std::vector<double> m_computeDispatchTimes{};
	std::vector<double> m_graphicsDispatchTimes{};

	// Method for settings settings depending on scene.
	void SetSettings();

	enum ComputeRootParameterIndices {
		SrvUavTable,
		ComputeRootParametersCount
	};

	enum ShaderDescriptorIndices {
		RTOutputResourceIdx,
		TLASIdx,
		CameraIdx,
		PerInstancePropsIdx,
		RenderSettingsIdx,
		ComputeShaderOutputIdx,
		ComputeConstantBufferIdx,
		ComputeTLASIdx,
		ComputeCubeVB,
		ComputeIndexedPlaneVB,
		ComputeCubeIndexBuffer,
		ComputeIndexedPlaneIndexBuffer,
		ComputePerInstancePropsIdx,
		ComputeSurfelBufferIdx,
		ComputeAABBIdx,
		NbShaderDescriptors
	};
	
	// Valid numbers of surfels per probe.
	enum NbSurfelsPerProbe {
		SURFELS_PER_PROBE_64 = 64,
		SURFELS_PER_PROBE_96 = 96,
		SURFELS_PER_PROBE_128 = 128,
		SURFELS_PER_PROBE_160 = 160,
	};

	enum Scenes {
		Scene0,
		Scene1
	};

	enum GIPreset {
		Off,
		NoSurfels,
		Full
	};

	Scenes m_scene{ Scene1 };
	GIPreset m_GIPreset{ Full };
	static const NbSurfelsPerProbe m_nbSurfelsPerProbe{ SURFELS_PER_PROBE_64 };
	bool m_MSAAOn{ true };
	int m_nbCubes{};
	int m_nbPlanes{};
	int m_nbBoxes{};
	int m_nbIndexedPlanes{};
	int m_nbProbeMarkers{};
	int m_nbLightMarkers{};
	float m_lightSpeed{ 3.f };
	float m_lightColor[3]{ 1.f, 1.f, 1.f };
	float m_lightPos[3]{}; //< Set in SetSettings().
	float m_lightSpread{}; //< Set in SetSettings().
	float m_ambientTerm{}; //< Set in SetSettings().
	float m_basePlaneSideLength{ 400.f };
	int m_nbProbesPerLayer{ 441 }; //< Must be the square of an integer!
	int m_nbProbeLayers{}; //< Set in SetSettings().
	float m_distBetweenProbes{ m_basePlaneSideLength / ((float)sqrt(m_nbProbesPerLayer) - 1)};
	bool m_useDebugProbesBlackAndWhite{ false };
	bool m_interpolateProbes{ true };
	bool m_useAdvancedLighting{ true };
	bool m_useProbeLighting{ true };
	bool m_useShadows{}; //< Set in SetSettings().
	bool m_animate{ false };
	bool m_updateProbes{ true };
	bool m_movingLight{ false };
	bool m_useOctagonalProbes{ true };
	bool m_useDirectionalLight{ false };
	float m_directionalLightAngle[3]{ 1.f, 1.f, -3.f };
	float m_bounceAttenuation{}; //< Set in SetSettings(). Higher means more light.
	bool m_renderProbeMarkers{ false };
	bool m_renderLightMarkers{ false };
	float m_falloffRegulator{}; //< Set in SetSettings().
	float m_brightness{}; //< Set in SetSettings().
	float m_multiBounceIntensity{}; //< Set in SetSettings().
	bool m_useMultiBounce{ false };
	float m_probeBrightness{}; //< Set in SetSettings().
	
	struct GlobalConstantBufferData {
		float lightPos[3];
		int	 frameCount;
		float lightColor[3];
		float	 lightIntensity;
		float	 ambientTerm;
		int		 nbProbesPerLayer; 
		int		 nbProbeLayers;
		float	 basePlaneSideLength;
		float directionalLightAngle[3];
		int		 interpolateProbes;		//< This is treated as a bool in the shaders. It's an int here for alignment.
		int		 useAdvancedLighting;   //< This is treated as a bool in the shaders. It's an int here for alignment.
		int		 useProbeLighting;		//< This is treated as a bool in the shaders. It's an int here for alignment.
		int		 useShadows;			//< This is treated as a bool in the shaders. It's an int here for alignment.
		int		 useOctagonalProbes;	//< This is treated as a bool in the shaders. It's an int here for alignment.
		int		 useDirectionalLight;	//< This is treated as a bool in the shaders. It's an int here for alignment.
		int		nbGeometryInstancesInScene;
		int		nbProbeMarkersInScene;
		float	probeBrightness;
	};

	struct ComputeConstantBufferData {
		int nbProbesPerLayer;
		int	nbProbeLayers;
		float basePlaneSideLength;
		int updateProbes;		//< This is treated as a bool in the shaders. It's an int here for alignment.
		int useOctagonalProbes; //< This is treated as a bool in the shaders. It's an int here for alignment.
		float lightPos[3];
		float lightColor[3];
		float lightIntensity;
		float bounceAttenuation;
		int useShadows;			 //< This is treated as a bool in the shaders. It's an int here for alignment.
		int useDirectionalLight; //< This is treated as a bool in the shaders. It's an int here for alignment.
		int	nbCubesInScene;
		float directionalLightAngle[3];
		int frameCount;
		int nbSurfelsPerProbe;
		float falloffRegulator;
		int	noSurfels; 			 //< This is treated as a bool in the shaders. It's an int here for alignment.
		float multiBounceIntensity;
		int useMultiBounce;		 //< This is treated as a bool in the shaders. It's an int here for alignment.
	};

	struct Surfel {
		float color[3];
		float position[3];
		float normal[3];
	};

	struct Probe {
		float colors[3][8];
	};

	struct AABB {
		float boundaries[6];
	};

	struct RenderSettings {
		int MSAAOn;				//< This is treated as a bool in the shaders. It's an int here for alignment.
		int renderProbeMarkers; //< This is treated as a bool in the shaders. It's an int here for alignment.
		int renderLightMarkers; //< This is treated as a bool in the shaders. It's an int here for alignment.
		float brightness;
	};

	float m_cubeScale{}; //< Set in SetSettings().
	float m_cubePosition[3]{ 0.f, m_distBetweenProbes + 10.f, 0.f };
	float m_doorPosition[3]{ 120.f, 17.f, 15.f };

	// Utility functions.
	bool XMMatrix4x4Compare(const XMMATRIX& m1, const XMMATRIX& m2) {
		for (uint8_t i{ 0 }; i < 4; i++) {
			for (uint8_t j{ 0 }; j < 4; j++) {
				if (m1.r[i].m128_f32[j] != m2.r[i].m128_f32[j])
					return false;
			}
		}
		return true;
	}

	AABB PointsToAABB(XMFLOAT4 points[8]) {
		AABB output{ -INFINITY, INFINITY, -INFINITY, INFINITY, -INFINITY, INFINITY };

		for (uint8_t i{ 0 }; i < 8; i++) {
			if (points[i].x > output.boundaries[0])
				output.boundaries[0] = points[i].x;
		}

		for (uint8_t i{ 0 }; i < 8; i++) {
			if (points[i].x < output.boundaries[1])
				output.boundaries[1] = points[i].x;
		}
		
		for (uint8_t i{ 0 }; i < 8; i++) {
			if (points[i].y > output.boundaries[2])
				output.boundaries[2] = points[i].y;
		}

		for (uint8_t i{ 0 }; i < 8; i++) {
			if (points[i].y < output.boundaries[3])
				output.boundaries[3] = points[i].y;
		}

		for (uint8_t i{ 0 }; i < 8; i++) {
			if (points[i].z > output.boundaries[4])
				output.boundaries[4] = points[i].z;
		}

		for (uint8_t i{ 0 }; i < 8; i++) {
			if (points[i].z < output.boundaries[5])
				output.boundaries[5] = points[i].z;
		}

		return output;
	}

	std::vector<XMFLOAT4> TransformPoints(Vertex points[8], XMMATRIX transform) {
		XMFLOAT4 transformedPoints[8]{};

		for (uint16_t i{ 0 }; i < 8; i++) {
			XMFLOAT3 pointAsVector = points[i].position;
			XMVECTOR pointAs4Vector{ pointAsVector.x, pointAsVector.y, pointAsVector.z, 1 };
			XMVECTOR transformedPointsVector = XMVector4Transform(pointAs4Vector, transform);

			XMStoreFloat4(&transformedPoints[i], transformedPointsVector);
		}

		std::vector<XMFLOAT4> out(transformedPoints, transformedPoints + sizeof(transformedPoints) / sizeof(transformedPoints[0]));
		return out;
	}
};