#include "stdafx.h"
#include "Thesis.h"

// Create the raytracing pipeline, associating the shader code to symbol names
// and to their root signatures, and defining the amount of memory carried by
// rays (ray payload)

//---------------------------------------------------------------------------------------------------------------------
// The raytracing pipeline binds the shader code, root signatures and pipeline characteristics in a single structure 
// used by DXR to invoke the shaders and manage temporary memory during raytracing.
// 
void Thesis::CreateRaytracingPipeline() {
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_device.Get());

	// The pipeline contains the DXIL code of all the shaders potentially executed during the raytracing process. This
	// section compiles the HLSL code into a set of DXIL libraries. We chose to separate the code in several libraries
	// by semantic (ray generation, hit, miss) for clarity. Any code layout can be used.
	m_rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
	m_missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
	m_hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");
	m_shadowLibrary = nv_helpers_dx12::CompileShaderLibrary(L"ShadowRay.hlsl");
	m_probeLibrary = nv_helpers_dx12::CompileShaderLibrary(L"ProbeHit.hlsl");

	// In a way similar to DLLs, each library is associated with a number of exported symbols. This has to be done 
	// explicitly in the lines below. Note that a single library can contain an arbitrary number of symbols, whose
	// semantic is given in HLSL using the [shader("xxx")] syntax.
	pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"RayGen" });
	pipeline.AddLibrary(m_missLibrary.Get(), { L"Miss" });
	pipeline.AddLibrary(m_hitLibrary.Get(), { L"ClosestHit", L"PlaneClosestHit" }); //< In the same file! 
	pipeline.AddLibrary(m_shadowLibrary.Get(), { L"ShadowClosestHit", L"ShadowMiss" });
	pipeline.AddLibrary(m_probeLibrary.Get(), { L"ProbePlaneClosestHit" });

	// To be used, each DX12 shader needs a root signature defining which parameters and buffers will be accessed. 
	m_rayGenSignature = CreateRayGenSignature();
	m_missSignature = CreateMissSignature();
	m_hitSignature = CreateHitSignature();
	m_shadowSignature = CreateHitSignature();
	m_probePlaneSignature = CreateProbePlaneHitSignature();

	// Hit group for the triangles, with a shader simply interpolating vertex colors.
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
	// Hit group for the ground plane.
	pipeline.AddHitGroup(L"PlaneHitGroup", L"PlaneClosestHit");
	// Hit group for all geometry when hit by a shadow ray.
	pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");
	// Hit group for probes.
	pipeline.AddHitGroup(L"ProbePlaneHitGroup", L"ProbePlaneClosestHit");

	// The following section associates the root signature to each shader. Note that we can explicitly show that some 
	// shaders share the same root signature (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and closest-hit shaders share the same root 
	// signature. 
	pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"RayGen" });
	// ShadowMiss has the same signature as the original miss shader.
	pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss", L"ShadowMiss" });
	pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"HitGroup", L"PlaneHitGroup" });
	pipeline.AddRootSignatureAssociation(m_shadowSignature.Get(), { L"ShadowHitGroup" });
	pipeline.AddRootSignatureAssociation(m_probePlaneSignature.Get(), { L"ProbePlaneHitGroup" });

	// The payload size defines the maximum size of the data carried by the rays, ie. the data exchanged between shaders, 
	// such as the HitInfo structure in the HLSL code. It is important to keep this value as low as possible as a too 
	// high value would result in unnecessary memory consumption and cache trashing.
	pipeline.SetMaxPayloadSize(4 * sizeof(float)); //< RGB + distance

	// Upon hitting a surface, DXR can provide several attributes to the hit. In our sample we just use the barycentric 
	// coordinates defined by the weights u,v of the last two vertices of the triangle. The actual barycentrics can
	// be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); //< barycentric coordinates

	// The raytracing process can shoot rays from existing hit points, resulting in nested TraceRay calls. Note that this 
	// recursion depth should be kept to a minimum for best performance. Path tracing algorithms can be easily flattened 
	// into a simple loop in the ray generation.
	pipeline.SetMaxRecursionDepth(2);

	// Compile the pipeline for execution on the GPU
	m_rtStateObject = pipeline.Generate();

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	ThrowIfFailed(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));
}