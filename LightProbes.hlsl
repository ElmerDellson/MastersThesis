#include "DataTypes.hlsl"
#include "Common.hlsl"
#include "PropageLightFromNeighbors.hlsl"

#ifndef LIGHT_PROBES
#define LIGHT_PROBES

Probe LightOctagonalProbes(uint3 DTid, float3 probePos, float distanceScaling, uint probeIndex, uint nbSurfelsPerProbe,
                           uint nbProbesPerLayer, uint nbProbeLayers, RWStructuredBuffer<Probe> probes, float3 lightPos,
                           float3 lightColor, bool useShadows, RaytracingAccelerationStructure SceneBVH,
                           float lightSpread, bool useDirectionalLight, float3 directionalLightAngle,
                           float bounceAttenuation, RWStructuredBuffer<Surfel> surfels, float basePlaneSideLength,
                           float multiBounceIntensity, bool useMultiBounce)
{
    Probe probe = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint sectionsInXZPlane = (uint)((float)nbSurfelsPerProbe / 8.0);
    float3 principalDirections[8] = { float3(1, 1, 0), float3(0, 1, 1), float3(-1, 1, 0), float3(0, 1, -1), 
        float3(1, -1, 0), float3(0, -1, 1), float3(-1, -1, 0), float3(0, -1, -1) };
    
    // Clamp probe indices to avoid going out of bounds when looking at neighbors.
    uint3 probeIndicesClamped = uint3(clamp(DTid.x, 1, (uint)sqrt(nbProbesPerLayer) - 2),
        clamp(DTid.y, 1, nbProbeLayers - 2),
        clamp(DTid.z, 1, (uint)sqrt(nbProbesPerLayer) - 2));

    // ProbeArray neighbors = Get8Neighborhood(probeIndicesClamped, nbProbesPerLayer, probes);
    
    for (int direction = 0; direction < 8; direction++)
    {
        float3 colorFromNeighborInDirection = PropagateLightFromNeighborsOctagonal(probeIndicesClamped, direction, distanceScaling,
                                                                                   nbProbesPerLayer, probes);

        for (uint i = 0; i < sectionsInXZPlane; i++)
        {
            Surfel surfel = surfels[probeIndex * nbSurfelsPerProbe + direction * sectionsInXZPlane + i];

            // Check if surfel is valid.
            if (!VectorCompare(surfel.normal, 0))
            {
                float3 toSurfel = surfel.position - probePos;
                float nDotL = max(dot(normalize(principalDirections[direction]), normalize(toSurfel)), 0);
                float3 advancedLighting = calculateAdvancedLighting(surfel.position, surfel.normal, lightPos, lightColor, 
                    useShadows, 0, SceneBVH, 0, lightSpread, useDirectionalLight, directionalLightAngle, 
                    DTid.xy * direction, DTid.yz);

                // Multiple bounces.
                if (useMultiBounce)
                    advancedLighting += calculateProbeLightingOctagonal(surfel.position, lightPos, nbProbesPerLayer,
                                                                        nbProbeLayers, basePlaneSideLength, true, 
                                                                        probes, surfel.normal) * multiBounceIntensity;

                float clampedBounceAttenuation = clamp(bounceAttenuation, 0, 1);

                // Attenuate based on distance between surfel and probe.
                advancedLighting /= max(length(toSurfel) * length(toSurfel), 1.0);

                probe.colors[direction] += (surfel.albedo * advancedLighting * nDotL * clampedBounceAttenuation) / 
                                                                                        sectionsInXZPlane; 
            }
            else
                probe.colors[direction] += (colorFromNeighborInDirection) / sectionsInXZPlane;
        }
    }

    return probe;
}

// Same as above, but reading the surfels from a SurfelBatch object. TODO: Merge these two functions.
Probe LightOctagonalProbes(uint3 DTid, float3 probePos, float distanceScaling, uint probeIndex, uint nbSurfelsPerProbe,
                           uint nbProbesPerLayer, uint nbProbeLayers, RWStructuredBuffer<Probe> probes, float3 lightPos,
                           float3 lightColor, bool useShadows, RaytracingAccelerationStructure SceneBVH,
                           float lightSpread, bool useDirectionalLight, float3 directionalLightAngle,
                           float bounceAttenuation, SurfelBatch64 surfelBatch, float basePlaneSideLength, 
                           bool useMultiBounce, float multiBounceIntensity)
{
    Probe probe = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint sectionsInXZPlane = (uint)((float)nbSurfelsPerProbe / 8.0);
    float3 principalDirections[8] = { float3(1, 1, 0), float3(0, 1, 1), float3(-1, 1, 0), float3(0, 1, -1),
                                      float3(1, -1, 0), float3(0, -1, 1), float3(-1, -1, 0), float3(0, -1, -1) };

    // Clamp probe indices to avoid going out of bounds when looking at neighbors.
    uint3 probeIndicesClamped = uint3(clamp(DTid.x, 1, (uint)sqrt(nbProbesPerLayer) - 2),
                                      clamp(DTid.y, 1, nbProbeLayers - 2),
                                      clamp(DTid.z, 1, (uint)sqrt(nbProbesPerLayer) - 2));

    ProbeArray neighbors = Get8Neighborhood(probeIndicesClamped, nbProbesPerLayer, probes);

    for (int direction = 0; direction < 8; direction++)
    {
        float3 colorFromNeighborInDirection = neighbors.probes[direction].colors[direction] * distanceScaling;

        for (uint i = 0; i < sectionsInXZPlane; i++)
        {
            Surfel surfel = surfelBatch.surfels[probeIndex * nbSurfelsPerProbe + direction * sectionsInXZPlane + i];

            // Check if surfel is valid.
            if (!VectorCompare(surfel.normal, 0))
            {
                float3 toSurfel = surfel.position - probePos;
                float nDotL = max(dot(normalize(principalDirections[direction]), normalize(toSurfel)), 0);
                float3 advancedLighting = calculateAdvancedLighting(surfel.position, surfel.normal, lightPos, lightColor,
                                                                    useShadows, 0, SceneBVH, 0, lightSpread, useDirectionalLight, directionalLightAngle,
                                                                    DTid.xy * direction, DTid.yz);

                // Multiple bounces.
                if (useMultiBounce)
                    advancedLighting += calculateProbeLightingOctagonal(surfel.position, lightPos, nbProbesPerLayer,
                                                                        nbProbeLayers, basePlaneSideLength, true,
                                                                        probes, surfel.normal) * multiBounceIntensity;

                float clampedBounceAttenuation = clamp(bounceAttenuation, 0, 1);

                // Attenuate based on distance between surfel and probe.
                advancedLighting /= max(length(toSurfel) * length(toSurfel), 1.0);

                probe.colors[direction] += (surfel.albedo * advancedLighting * nDotL * clampedBounceAttenuation) /
                                           sectionsInXZPlane;
            }
            else
                probe.colors[direction] += (colorFromNeighborInDirection) / sectionsInXZPlane;
        }
    }

    return probe;
}

#endif