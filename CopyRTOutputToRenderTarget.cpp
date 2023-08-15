#include "stdafx.h"
#include "Thesis.h"

void Thesis::CopyRTOutputToRenderTarget(CD3DX12_RESOURCE_BARRIER& transition)
{
	// The raytracing output needs to be copied to the actual render target used for display. For this, we need to 
	// transition the raytracing output from a UAV to a copy source, and render target buffer to a copy destination.
	// We can then do the actual copy, before transitioning the render target buffer into a render target, that will
	// then be used to display the image.
	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_RTOutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_graphicsCommandList->ResourceBarrier(1, &transition);

	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	m_graphicsCommandList->ResourceBarrier(1, &transition);
	m_graphicsCommandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_RTOutputResource.Get());

	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_graphicsCommandList->ResourceBarrier(1, &transition);
}