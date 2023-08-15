#include "Common.hlsl"

float3 RenderProbeMarker(float3 hitWorldPos);
float3 RenderLightMarker();

cbuffer GlobalConstants : register(b0) {
    float3 lightPos;
    uint frameCount;
    float3 lightColor;
    float lightSpread;
    float ambientTerm;
    int nbProbesPerLayer;
    int nbProbeLayers;
    float basePlaneSideLength;
    float3 directionalLightAngle;
    bool interpolateProbes;
    bool lightOn;
    bool useProbeLighting;
    bool useShadows;
    bool useOctagonalProbes;
    bool useDirectionalLight;
    int nbGeometryInstancesInScene;
    int nbProbeMarkersInScene;
    float probeBrightness;
}

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices: register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2); //< Raytracing acceleration structure, accessed as a SRV.
StructuredBuffer<InstanceProperties> instanceProps : register(t3);
StructuredBuffer<Probe> probes : register(t4);

// #DXR Extra: Another ray type.
[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
    float3 hitWorldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    // If hit is not on a probe marker.
    if (InstanceID() < nbGeometryInstancesInScene)
    {    
        // Albedo.
        float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
        uint vertId = 3 * PrimitiveIndex();
        float3 hitColor = BTriVertex[indices[vertId + 0]].color.xyz * barycentrics.x + 
            BTriVertex[indices[vertId + 1]].color.xyz * barycentrics.y +
            BTriVertex[indices[vertId + 2]].color.xyz * barycentrics.z;
    
        float3 e1 = BTriVertex[indices[vertId + 1]].vertex - BTriVertex[indices[vertId + 0]].vertex;
        float3 e2 = BTriVertex[indices[vertId + 2]].vertex - BTriVertex[indices[vertId + 0]].vertex;
        float3 normal = mul(instanceProps[InstanceID()].objectToWorldNormal, float4(normalize(cross(e2, e1)), 0.f)).xyz;
    
        // Phong, shadows and falloff.
        float3 lightColor2 = lightOn ? lightColor : float3(0, 0, 0);

        hitColor *= calculateAdvancedLighting(hitWorldPos, normal, lightPos, lightColor2, useShadows, ambientTerm,
                                              SceneBVH, frameCount, lightSpread, useDirectionalLight, 
                                              directionalLightAngle, DispatchRaysIndex().xy, DispatchRaysDimensions().xy);
    
        // Probes (first order bounce).
        if (useProbeLighting)
        {
            hitColor += calculateProbeLightingOctagonal(hitWorldPos, lightPos, nbProbesPerLayer, nbProbeLayers,
                                                        basePlaneSideLength, interpolateProbes, probes, normal, 
                                                        probeBrightness);
        }
    
        payload.colorAndDistance = float4(hitColor, RayTCurrent());
    }
    else if (InstanceID() < nbGeometryInstancesInScene + nbProbeMarkersInScene)
        payload.colorAndDistance = float4(RenderProbeMarker(hitWorldPos), RayTCurrent());
    else
        payload.colorAndDistance = float4(RenderLightMarker(), RayTCurrent());
}

float3 RenderLightMarker()
{
    return lightColor;
}

float3 RenderProbeMarker(float3 hitWorldPos)
{
    float stepSize = basePlaneSideLength / (sqrt(nbProbesPerLayer) - 1);
    float x = min(max(hitWorldPos.x + basePlaneSideLength / 2, 0), basePlaneSideLength);
    float z = min(max(hitWorldPos.z + basePlaneSideLength / 2, 0), basePlaneSideLength);
    float y = clamp(hitWorldPos.y, 0, nbProbeLayers * stepSize);
    
    // Round coordinates to nearest stepsize.
    float xRounded = round((x + stepSize / 2) - ((x + stepSize / 2) % stepSize));
    float zRounded = round((z + stepSize / 2) - ((z + stepSize / 2) % stepSize));
    float yRounded = round((y + stepSize / 2) - ((y + stepSize / 2) % stepSize));

    int probeX = (int)((xRounded / stepSize) * sqrt(nbProbesPerLayer));
    int probeY = (int)((yRounded / stepSize) * nbProbesPerLayer);
    int probeZ = (int)(zRounded / stepSize);

    // return probes[probeX + probeY + probeZ].colors[PrimitiveIndex()] * 10.0;
    return float3(1, 1, 1);
}

// TODO: Delete this without breaking the root signature.
[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
}

