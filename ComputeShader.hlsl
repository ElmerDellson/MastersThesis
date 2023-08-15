#include "Common.hlsl"
#include "PropageLightFromNeighbors.hlsl"
#include "LightProbes.hlsl"
#include "PlaceSurfels.hlsl"

#ifndef COMPUTE_SHADER
#define COMPUTE_SHADER

cbuffer ComputeConstants : register(b0)
{
    uint nbProbesPerLayer;
    uint nbProbeLayers;
    float basePlaneSideLength;
    bool updateProbes;
    bool useOctagonalProbes;
    float3 lightPos;
    float3 lightColor;
    float lightSpread;
    float bounceAttenuation;
    bool useShadows;
    bool useDirectionalLight;
    int nbCubesInScene;
    float3 directionalLightAngle;
    int frameCount;
    uint nbSurfelsPerProbe;
    float falloffRegulator;
    bool noSurfels;
    float multiBounceIntensity;
    bool useMultiBounce;
}

RWStructuredBuffer<Probe> probes : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0); //< Raytracing acceleration structure, accessed as a SRV.
StructuredBuffer<STriVertex> CubeBTriVertex : register(t1);
StructuredBuffer<STriVertex> IndexedPlaneBTriVertex : register(t2);
StructuredBuffer<int> CubeIndices : register(t3);
StructuredBuffer<int> IndexedPlaneIndices : register(t4);
StructuredBuffer<InstanceProperties> instanceProps : register(t5);
RWStructuredBuffer<Surfel> surfels : register(u1);
StructuredBuffer<AABB> AABBs : register(t6);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint y = DTid.y * nbProbesPerLayer;
    uint x = DTid.x * (uint)sqrt(nbProbesPerLayer);
    uint z = DTid.z;
    float distBetweenProbes = basePlaneSideLength / (sqrt(nbProbesPerLayer) - 1);
    float distanceScaling = max(falloffRegulator / (2 * distBetweenProbes * distBetweenProbes), 0);
    float3 probePos = DTid * distBetweenProbes - float3(basePlaneSideLength / 2, 0, basePlaneSideLength / 2);

    if (noSurfels)
    {
        SurfelBatch64 surfelBatch = PlaceSurfelsAndReturn(probePos, distBetweenProbes, x + y + z, nbSurfelsPerProbe,
            SceneBVH, nbCubesInScene, CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex, IndexedPlaneIndices,
            instanceProps);

        probes[x + y + z] = LightOctagonalProbes(DTid, probePos, distanceScaling, x + y + z, nbSurfelsPerProbe, 
                                                nbProbesPerLayer, nbProbeLayers, probes, lightPos, lightColor, 
                                                useShadows, SceneBVH, lightSpread, useDirectionalLight, 
                                                directionalLightAngle, bounceAttenuation, surfelBatch, 
                                                basePlaneSideLength, useMultiBounce, multiBounceIntensity);
    }
    else
    {
        if (updateProbes)
            PlaceSurfels(probePos, distBetweenProbes, x + y + z, nbSurfelsPerProbe, surfels, SceneBVH, nbCubesInScene, 
                        CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex, IndexedPlaneIndices, instanceProps);
        else
        {
            for (uint i = 0; i < nbCubesInScene * 2; i++)
            {
                if (CheckAABB(probePos, AABBs[i], distBetweenProbes))
                {
                    PlaceSurfels(probePos, distBetweenProbes, x + y + z, nbSurfelsPerProbe, surfels, SceneBVH, nbCubesInScene,
                                 CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex, IndexedPlaneIndices, instanceProps);
                    break;
                }
            }
        }

        probes[x + y + z] = LightOctagonalProbes(DTid, probePos, distanceScaling, x + y + z, nbSurfelsPerProbe,
                                                 nbProbesPerLayer, nbProbeLayers, probes, lightPos, lightColor, useShadows,
                                                 SceneBVH, lightSpread, useDirectionalLight, directionalLightAngle,
                                                 bounceAttenuation, surfels, basePlaneSideLength, multiBounceIntensity,
                                                 useMultiBounce);
    }
}

#endif