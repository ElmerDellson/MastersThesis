#include "stdafx.h"
#include "Thesis.h"

ComPtr<ID3D12RootSignature> Thesis::CreateRayGenSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;

	rsc.AddHeapRangesParameter({ {0 /*u0*/, 1 /*1 descriptor*/, 0 /*use the implicit space 0*/,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV /*UAV representing the output buffer*/, 0 /*heap slot where the UAV is defined*/},
		{0 /*t0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/, 1},
		{0 /*b0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Camera parameters*/, 2},
		{1 /*b1*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Settings*/, 4} });

	return rsc.Generate(m_device.Get(), true);
}

ComPtr<ID3D12RootSignature> Thesis::CreateHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0 /*t0*/); // Vertices and colors
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1 /*t1*/); // Indices

	// #DXR Extra: Per-Instance Data
	// The vertex colors may differ for each instance, so it is not possible to point to a single buffer in the heap.
	// Instead, we use the concept of root parameters, which are defined directly by a pointer in memory. In the shader
	// binding table we will associate each hit shader instance with its constant buffer. Here we bind the buffer to 
	// the first slot, accessible in HLSL as register(b0).
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);

	// #DXR Extra: Another ray type
	rsc.AddHeapRangesParameter({ {2 /*t2*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 /*2nd slot of the heap*/ },
		{1 /*b1*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Scene data*/, 2},
		// #DXR Extra: Simple lighting
		{3 /*t3*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Per-instance data*/, 3},
		});

	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 4 /*t4*/); // Probes

	return rsc.Generate(m_device.Get(), true);
}

ComPtr<ID3D12RootSignature> Thesis::CreateProbePlaneHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0 /*t0*/); // Vertices and colors
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1 /*t1*/); // Probes

	// #DXR Extra: Per-Instance Data
	// The vertex colors may differ for each instance, so it is not possible to point to a single buffer in the heap.
	// Instead, we use the concept of root parameters, which are defined directly by a pointer in memory. In the shader
	// binding table we will associate each hit shader instance with its constant buffer. Here we bind the buffer to 
	// the first slot, accessible in HLSL as register(b0).
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);

	// #DXR Extra: Another ray type
	rsc.AddHeapRangesParameter({ {2 /*t2*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1 /*2nd slot of the heap*/ },
		{1 /*b1*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Scene data*/, 2},
		// #DXR Extra: Simple lighting
		{3 /*t3*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Per-instance data*/, 3},
		});

	return rsc.Generate(m_device.Get(), true);
}

ComPtr<ID3D12RootSignature> Thesis::CreateMissSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;

	return rsc.Generate(m_device.Get(), true);
}