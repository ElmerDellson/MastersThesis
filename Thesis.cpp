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

#include "stdafx.h"
#include "Thesis.h"

Thesis::Thesis(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorHeapSize(0) { }

void Thesis::OnInit()
{
	SetSettings();

	UINT dxgiFactoryFlags{ 0 };
	ComPtr<IDXGIFactory4> factory;
	//EnableDebugLayer(dxgiFactoryFlags);
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
	CreateDevice(factory);
	CheckRaytracingSupport();

	LoadComputePipeline();
	LoadGraphicsPipeline(factory);
	LoadAssets();

	CreateAccelerationStructures();
	CreateRaytracingPipeline();

	CreateQueryHeapAndResultBuffer();
	CreateGlobalConstantBuffer();
	CreateRaytracingOutputBuffer();
	CreateInstancePropertiesBuffer();
	CreateCameraBuffer();
	CreateRenderSettingsBuffer();
	CreateOutputAndReadbackBuffers();
	CreateSurfelBuffer();
	CreateComputeConstantBuffer();
	CreateAABBBuffer();

	CreateShaderDescriptorHeap();
	CreateShaderBindingTable();
}

void Thesis::LoadAssets()
{
	// Create the vertex buffers.
	CreateCubeVB();
	CreateIndexedPlanesVB();
	CreateProbeMarkerVB();

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	CreateSyncObjectsAndWait();
}  

// Update frame-based values.
void Thesis::OnUpdate()
{
	UpdateCameraBuffer();
	UpdateRenderSettingsBuffer();
	UpdateGlobalConstantBuffer();
	UpdateComputeConstantBuffer();
	UpdateAABBBuffer();

	// #DXR Extra: Refitting
	UpdateInstancePropertiesBuffer();

	m_frameCount++;
	HandleAnimation(false);

	if (m_updateProbes)
		m_updateProbes = false;

	HandleReadbackBuffers();
}

// Render the scene.
void Thesis::OnRender() {
	// Record commands for the compute shader.
	if (m_GIPreset != Off)
		RecordComputeCommands();

	// Record all the commands we need to render the scene into the command list.
	RecordGraphicsCommands();

	// Execute the compute command list.
	ID3D12CommandList* ppComputeCommandLists[] = { m_computeCommandList.Get() };
	m_computeCommandQueue->ExecuteCommandLists(_countof(ppComputeCommandLists), ppComputeCommandLists);

	m_fenceValue++;
	m_computeCommandQueue->Signal(m_fence.Get(), m_fenceValue);
	m_graphicsCommandQueue->Wait(m_fence.Get(), m_fenceValue);

	// Execute the graphics command list.
	ID3D12CommandList* ppGraphicsCommandLists[] = { m_graphicsCommandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppGraphicsCommandLists), ppGraphicsCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void Thesis::OnDestroy()
{
	ExportTimings();

	// Ensure that the GPU is no longer referencing resources that are about to be cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}