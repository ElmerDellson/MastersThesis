#include "Common.hlsl"

cbuffer GlobalConstants : register(b0)
{
    float3 lightPos;
    uint frameCount;
    float3 lightColor;
    float lightIntensity;
    float ambientTerm;
    int nbProbesPerLayer;  
    int nbProbeLayers;
    float basePlaneSideLength;
    bool interpolateProbes;
    bool useAdvancedLighting;
    bool useProbeLighting;
    bool useShadows;
}

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<Probe> probes : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2); //< Raytracing acceleration structure, accessed as a SRV.
StructuredBuffer<InstanceProperties> instanceProps : register(t3);

[shader("closesthit")]
void ProbePlaneClosestHit(inout HitInfo payload, Attributes attrib)
{
    //float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    //float3 hitWorldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection(); //< In world space.
    //uint vertId = 3 * PrimitiveIndex();        
    //float3 hitColor = BTriVertex[vertId + 0].color * barycentrics.x + BTriVertex[vertId + 1].color * barycentrics.y +
    //    BTriVertex[vertId + 2].color * barycentrics.z;
    
    //if (useProbeLighting)
    //    hitColor *= calculateProbeLighting(hitWorldPos, nbProbesPerLayer, nbProbeLayers, basePlaneSideLength, 
    //        interpolateProbes, probes);
    
    //if (useAdvancedLighting)
    //{
    //    float3 e1 = BTriVertex[vertId + 1].vertex - BTriVertex[vertId + 0].vertex;
    //    float3 e2 = BTriVertex[vertId + 2].vertex - BTriVertex[vertId + 0].vertex;
    //    float3 normal = mul(instanceProps[InstanceID()].objectToWorldNormal, float4(normalize(cross(e2, e1)), 0.f)).xyz;
        
    //    hitColor *= calculateAdvancedLighting(hitWorldPos, vertId, normal, lightPos, lightColor, useShadows, ambientTerm,
    //    SceneBVH, frameCount, lightIntensity);
    //}
    
    payload.colorAndDistance = float4(float3(0,0,0), RayTCurrent());
}