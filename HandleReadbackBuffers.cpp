#include "stdafx.h"
#include "Thesis.h"

void Thesis::HandleReadbackBuffers() {
	// Read content of probe buffer.
	D3D12_RANGE readbackBufferRange{ 0, m_computeProbeBufferSize };
	Probe* pReadbackBufferData{};
	ThrowIfFailed(m_readbackBuffer->Map(0, &readbackBufferRange, reinterpret_cast<void**>(&pReadbackBufferData)));

	// Read content of query readback buffer.
	D3D12_RANGE queryResultRange{ 0, 4 * sizeof(uint64_t) };
	uint64_t* pQueryData{};
	ThrowIfFailed(m_queryResultBuffer->Map(0, &queryResultRange, reinterpret_cast<void**>(&pQueryData)));

	// Get and process compute times.
	uint64_t freq;
	m_computeCommandQueue->GetTimestampFrequency(&freq);
	uint64_t ticksElapsed = (pQueryData[1] - pQueryData[0]);
	double timeElapsed = static_cast<double>(ticksElapsed) / (freq / 1000.f);
	m_computeDispatchTimes.push_back(timeElapsed);

	// Get and process graphics times.
	m_graphicsCommandQueue->GetTimestampFrequency(&freq);
	ticksElapsed = (pQueryData[3] - pQueryData[2]);
	timeElapsed = static_cast<double>(ticksElapsed) / (freq / 1000.f);
	m_graphicsDispatchTimes.push_back(timeElapsed);

	D3D12_RANGE emptyRange{ 0, 0 };
	m_readbackBuffer->Unmap(0, &emptyRange);
	m_queryResultBuffer->Unmap(0, &emptyRange);

	queryIndex += 4;
	if (queryIndex > 11) queryIndex = 0;
}