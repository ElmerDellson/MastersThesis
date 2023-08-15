#include "Common.hlsl"

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float ramp = launchIndex.y / dims.y;

    float3 color = float3(0.0f, 0.2f, 0.7f - 0.3 * ramp);
    float3 black = float3(0, 0, 0);
    
    payload.colorAndDistance = float4(black, -1.f);
}