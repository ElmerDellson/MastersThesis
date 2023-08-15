#include "Common.hlsl"

float3 DoRayTrace(float2 fudge = 0);

// #DXR Extra: Perspective Camera
cbuffer CameraParams : register(b0) {
	float4x4 view;
	float4x4 projection;
	float4x4 viewI;
	float4x4 projectionI;
}

cbuffer Settings : register(b1)
{
    bool MSAAOn;
    bool renderProbeMarkers;
    bool renderLightMarkers;
    float brightness;
}

// Raytracing output texture, accessed as a UAV
RWTexture2D<float4> gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")] 
void RayGen() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    float3 color = 0;
	
    if (MSAAOn)
    {
        float fudge = 0.5;
	
        color += DoRayTrace(float2(fudge, 0));
        color += DoRayTrace(float2(0, -fudge));
        color += DoRayTrace(float2(0, fudge));
        color += DoRayTrace(float2(-fudge, 0));
        color /= 4;
    }
	else 	
		color = DoRayTrace(0);
	  
    gOutput[launchIndex] = float4(color * brightness, 1.f);
}

float3 DoRayTrace(float2 fudge)
{
	// Initialize the ray payload
    HitInfo payload;
    payload.colorAndDistance = float4(0, 0, 0, 0);

	// Get the location within the dispatched 2D grid of work items (often maps to pixels, so this could represent a 
	// pixel coordinate).
    float2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + fudge + 0.5f) / dims.xy) * 2.f - 1.f);

	// Define a ray, consisting of origin, direction, and the min-max distance values.
	// #DXR Extra : Perspective Camera 
    float aspectRatio = dims.x / dims.y;
    float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));
  
    RayDesc ray;
    ray.Origin = mul(viewI, float4(0, 0, 0, 1)).xyz;
    ray.Direction = mul(viewI, float4(target.xyz, 0)).xyz;
    ray.TMin = 0;
    ray.TMax = 100000;

    uint lightMarkersMask = renderLightMarkers ? 0x08 : 0x00;
    uint probeMarkersMask = renderProbeMarkers ? 0x04 : 0x00;
    uint instanceInclusionMask = 0x01 | 0x02 | probeMarkersMask | lightMarkersMask;
	
	// Trace the ray
	TraceRay(
		// Parameter name: AccelerationStructure
		// Acceleration structure.
		SceneBVH,
		// Parameter name: RayFlags
		// Flags can be used to specify the behavior upon hitting a surface.
		RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
		// Parameter name: InstanceInclusionMask
		// Instance inclusion mask, which can be used to mask out some geometry to this ray by and-ing the mask with a
		// geometry mask. The 0xFF flag then indicates no geometry will be masked.
		instanceInclusionMask,
		// Parameter name: RayContributionToHitGroupIndex
		// Depending on the type of ray, a given object can have several hit groups attached (i.e. what to do when hitting
		// to compute regular shading, and what to do when hitting to compute shadows). Those hit groups are specified
		// sequentially in the SBT, so the value below indicates which offset (on 4 bits) to apply to the hit groups for
		// this ray. In this sample we only have one hit group per object, hence an offset of 0.
		0,
		// Parameter name: MultiplierForGeometryContributionToHitGroupIndex
		// The offsets in the SBT can be computed from the object ID, its instance ID, but also simply by the order the
		// objects have been pushed in the acceleration structure. This allows the application to group shaders in the 
		// SBT in the same order as they are added in the AS, in which case the value below represents the stride (4 bits
		// representing the number of hit groups) between two consecutive objects.
		0,
		// Parameter name: MissShaderIndex
		// Index of the miss shader to use in case several consecutive miss shaders are present in the SBT. This allows 
		// to change the behavior of the program when no geometry has been hit, for example one to return a sky color 
		// for regular rendering, and another returning a full visibility value for shadow rays. This sample only has one
		// miss shader, hence an index of 0.
		0,
		// Parameter name: Ray
		// Ray information to trace.
		ray,
		// Parameter name: Payload
		// Payload associated to the ray, which will be used to communicate between the hit/miss shaders and the raygen
		payload
  	);
	
    return payload.colorAndDistance.xyz;
}
