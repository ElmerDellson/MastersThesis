#include "Common.hlsl"
static const float pi = 3.14159265358979;

struct SurfelBatch64
{
    Surfel surfels[64];
};

struct AABB
{
    float boundaries[6]; // Right, left, front, back, up, down
};

SurfelBatch64 PlaceSurfelsAndReturn(float3 probePos, float surfelDistLimit, uint probeIndex);
void PlaceSurfels(float3 probePos, float surfelDistLimit, uint probeIndex);
Probe LightOctagonalProbes(uint3 DTid, float3 probePos, float distanceScaling, uint probeIndex);
Probe LightOctagonalProbes(uint3 DTid, float3 probePos, float distanceScaling, uint probeIndex, SurfelBatch64 surfelBatch);
Probe CubeProbes(uint3 DTid, float distBetweenProbes, float3 worldPos, float distanceScaling);
float3 PropagateLightFromNeighborsCube(float3 probeIndicesClamped, int direction, float distanceScaling);
float3 PropagateLightFromNeighborsOctagonal(uint3 probeIndicesClamped, int direction, float distanceScaling);

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
    int nbSurfelsPerProbe;
    float falloffRegulator;
    bool noSurfels;
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

bool CheckAABB(float3 p, AABB AABBIn, float bias)
{
    return (p.x < AABBIn.boundaries[0] + bias && p.x > AABBIn.boundaries[1] - bias) &&
           (p.y < AABBIn.boundaries[2] + bias && p.y > AABBIn.boundaries[3] - bias) &&
           (p.z < AABBIn.boundaries[4] + bias && p.z > AABBIn.boundaries[5] - bias); 
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint y = DTid.y * nbProbesPerLayer;
    uint x = DTid.x * (uint)sqrt(nbProbesPerLayer);
    uint z = DTid.z;
    float distBetweenProbes = basePlaneSideLength / (sqrt(nbProbesPerLayer) - 1);
    float surfelPlacementDist = distBetweenProbes * sqrt(2);
    float distanceScaling = max(1 / (2 * distBetweenProbes * distBetweenProbes / falloffRegulator), 0);
    float3 probePos = DTid * distBetweenProbes - float3(basePlaneSideLength / 2, 0, basePlaneSideLength / 2);

    if (noSurfels)
    {
        SurfelBatch64 surfelBatch = PlaceSurfelsAndReturn(probePos, surfelPlacementDist, x + y + z);
        probes[x + y + z] = LightOctagonalProbes(DTid, probePos, distanceScaling, x + y + z, surfelBatch);
    }
    else
    {
        if (updateProbes)
            PlaceSurfels(probePos, surfelPlacementDist, x + y + z);
        else
        {
            for (uint i = 0; i < nbCubesInScene * 2; i++)
            {
                if (CheckAABB(probePos, AABBs[i], surfelPlacementDist))
                {
                    PlaceSurfels(probePos, surfelPlacementDist, x + y + z);
                    break;
                }
            }
        }
    
        probes[x + y + z] = LightOctagonalProbes(DTid, probePos, distanceScaling, x + y + z);
    }
}

Surfel PlaceSurfel(float3 probePos, float distBetweenProbesDiag, uint i, uint j)
{
    float sectionsInXZPlane = (float)nbSurfelsPerProbe / 8;
    float sectionWidth = (2 * pi / sectionsInXZPlane);
    float adjustment = ((float)nbSurfelsPerProbe - 32) / 64;
    float phi = pi * (float(j) / 8) + (pi / 16);
    float theta = 2 * pi * (float(i) / sectionsInXZPlane) - (sectionWidth * adjustment);
    float3 direction = float3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));

    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = probePos;
    ray.TMin = 0.001;
    ray.TMax = distBetweenProbesDiag;
    ray.Direction = direction;
    q.TraceRayInline(SceneBVH, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, 0x01 | 0x02, ray);
    q.Proceed();

    Surfel surfel = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0) };
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        // Get color from hit, attenuate by ray length.
        float3 hitWorldPos = probePos + ray.Direction * q.CommittedRayT();
        float2 bary = q.CommittedTriangleBarycentrics();
        float3 barycentrics = float3(1.f - bary.x - bary.y, bary.x, bary.y);
        uint vertId = 3 * q.CommittedPrimitiveIndex();
        uint instanceId = q.CommittedInstanceID();

        float3 hitColor = 0;
        float3 e1, e2 = 0;
        if (instanceId < nbCubesInScene) {
            hitColor = CubeBTriVertex[CubeIndices[vertId + 0]].color.xyz * barycentrics.x +
                       CubeBTriVertex[CubeIndices[vertId + 1]].color.xyz * barycentrics.y +
                       CubeBTriVertex[CubeIndices[vertId + 2]].color.xyz * barycentrics.z;

            e1 = CubeBTriVertex[CubeIndices[vertId + 1]].vertex -
                 CubeBTriVertex[CubeIndices[vertId + 0]].vertex;
            e2 = CubeBTriVertex[CubeIndices[vertId + 2]].vertex -
                 CubeBTriVertex[CubeIndices[vertId + 0]].vertex;
        } else {
            hitColor = IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 0]].color.xyz * barycentrics.x +
                       IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 1]].color.xyz * barycentrics.y +
                       IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 2]].color.xyz * barycentrics.z;

            e1 = IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 1]].vertex -
                 IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 0]].vertex;
            e2 = IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 2]].vertex -
                 IndexedPlaneBTriVertex[IndexedPlaneIndices[vertId + 0]].vertex;
        }

        float3 normal = mul(instanceProps[instanceId].objectToWorldNormal, float4(normalize(cross(e2, e1)), 0.f)).xyz;

        surfel.color = hitColor;
        surfel.position = hitWorldPos;
        surfel.normal = normal;
    }
    else
        surfel.normal = float3(0, 0, 0);

    return surfel;
}

void PlaceSurfels(float3 probePos, float surfelDistLimit, uint probeIndex) {
    int sectionsInXZPlane = nbSurfelsPerProbe / 8;

    // Upper half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfels[probeIndex * nbSurfelsPerProbe + i * 4 + j] = PlaceSurfel(probePos, surfelDistLimit, i, j);
    }

    // Lower half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfels[probeIndex * nbSurfelsPerProbe + (nbSurfelsPerProbe / 2) + i * 4 + j] =
                PlaceSurfel(probePos, surfelDistLimit, i, j + 4);
    }
}

SurfelBatch64 PlaceSurfelsAndReturn(float3 probePos, float surfelDistLimit, uint probeIndex) {
    SurfelBatch64 surfelBatch;
    int sectionsInXZPlane = nbSurfelsPerProbe / 8;

    // Upper half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfelBatch.surfels[i * 4 + j] = PlaceSurfel(probePos, surfelDistLimit, i, j);
    }

    // Lower half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfelBatch.surfels[(nbSurfelsPerProbe / 2) + i * 4 + j] =
                PlaceSurfel(probePos, surfelDistLimit, i, j + 4);
    }

    return surfelBatch;
}

Probe LightOctagonalProbes(uint3 DTid, float3 probePos, float distanceScaling, uint probeIndex)
{
    Probe p = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int sectionsInXZPlane = nbSurfelsPerProbe / 8;
    float3 cardinalDirections[8] = { float3(1, 1, 0), float3(0, 1, 1), float3(-1, 1, 0), float3(0, 1, -1), 
        float3(1, -1, 0), float3(0, -1, 1), float3(-1, -1, 0), float3(0, -1, -1) };
        
    uint3 probeIndicesClamped = uint3(clamp(DTid.x, 1, (uint)sqrt(nbProbesPerLayer) - 2),
        clamp(DTid.y, 1, nbProbeLayers - 2),
        clamp(DTid.z, 1, (uint)sqrt(nbProbesPerLayer) - 2));

    for (int direction = 0; direction < 8; direction++)
    {
        float3 colorFromNeighborInDirection = PropagateLightFromNeighborsOctagonal(probeIndicesClamped, direction, distanceScaling);

        for (uint i = 0; i < sectionsInXZPlane; i++)
        {
            Surfel surfel = surfels[probeIndex * nbSurfelsPerProbe + direction * sectionsInXZPlane + i];

            if (!VectorCompare(surfel.normal, 0))
            {
                float3 toSurfel = surfel.position - probePos;
                float nDotL = clamp(dot(cardinalDirections[direction], toSurfel), 0, 1);
                float3 advancedLighting = calculateAdvancedLighting(surfel.position, surfel.normal, lightPos, lightColor, useShadows, 0, SceneBVH,
                                                                    0, lightSpread, useDirectionalLight, directionalLightAngle,
                                                                    DTid.xy * direction, DTid.yz);

                float clampedBounceAttenuation = clamp(bounceAttenuation, 0, 1);
                p.colors[direction] += (surfel.color * advancedLighting * nDotL * clampedBounceAttenuation) / sectionsInXZPlane; 
            }
            else
                p.colors[direction] += (colorFromNeighborInDirection) / sectionsInXZPlane;
        }
    }

    return p;
}

Probe LightOctagonalProbes(uint3 DTid, float3 probePos, float distanceScaling, uint probeIndex, SurfelBatch64 surfelBatch)
{
    Probe p = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int sectionsInXZPlane = nbSurfelsPerProbe / 8;
    float3 cardinalDirections[8] = { float3(1, 1, 0), float3(0, 1, 1), float3(-1, 1, 0), float3(0, 1, -1),
                                     float3(1, -1, 0), float3(0, -1, 1), float3(-1, -1, 0), float3(0, -1, -1) };

    uint3 probeIndicesClamped = uint3(clamp(DTid.x, 1, (uint)sqrt(nbProbesPerLayer) - 2),
                                      clamp(DTid.y, 1, nbProbeLayers - 2),
                                      clamp(DTid.z, 1, (uint)sqrt(nbProbesPerLayer) - 2));

    for (int direction = 0; direction < 8; direction++)
    {
        float3 colorFromNeighborInDirection = PropagateLightFromNeighborsOctagonal(probeIndicesClamped, direction, distanceScaling);

        for (uint i = 0; i < sectionsInXZPlane; i++)
        {
            Surfel surfel = surfelBatch.surfels[direction * sectionsInXZPlane + i];

            if (!VectorCompare(surfel.normal, 0))
            {
                float3 toSurfel = surfel.position - probePos;
                float nDotL = clamp(dot(normalize(cardinalDirections[direction]), normalize(toSurfel)), 0, 1);
                float3 advancedLighting = calculateAdvancedLighting(surfel.position, surfel.normal, lightPos, lightColor, useShadows, 0, SceneBVH,
                                                                    0, lightSpread, useDirectionalLight, directionalLightAngle,
                                                                    DTid.xy * direction, DTid.yz);

                float clampedBounceAttenuation = clamp(bounceAttenuation, 0, 1);
                p.colors[direction] += (surfel.color * advancedLighting * nDotL * clampedBounceAttenuation) / sectionsInXZPlane;
            }
            else
                p.colors[direction] += (colorFromNeighborInDirection) / sectionsInXZPlane;
        }
    }

    return p;
}

float3 PropagateLightFromNeighborsOctagonal(uint3 probeIndicesClamped, int direction, float distanceScaling)
{
    // Propagate probe lighting by checking the color from direction of the neighboring probe in the current direction.
    switch (direction)
    {
        case 0: // Upper left.
            return probes[(probeIndicesClamped.x + 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[0] * distanceScaling;
        case 1: // Upper front.
            return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z + 1].colors[1] * distanceScaling;
        case 2: // Upper right.
            return probes[(probeIndicesClamped.x - 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[2] * distanceScaling;
        case 3: // Upper back.
            return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z - 1].colors[3] * distanceScaling;
        case 4: // Lower left.
            return probes[(probeIndicesClamped.x + 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[4] * distanceScaling;
        case 5: // Lower front.
            return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z + 1].colors[5] * distanceScaling;
        case 6: // Lower right.
            return probes[(probeIndicesClamped.x - 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[6] * distanceScaling;
        default: // Lower back.
            return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z - 1].colors[7] * distanceScaling;
    }
}

float3 PropagateLightFromNeighborsCube(float3 probeIndicesClamped, int direction, float distanceScaling)
{
    // Propagate probe lighting by checking the color from direction of the neighboring probe in the current direction.
    switch (direction)
    {
        case 0: // From up
            return probes[probeIndicesClamped.x * sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) * nbProbesPerLayer + probeIndicesClamped.z].colors[0] * distanceScaling;
        case 1: // From Down
            return probes[probeIndicesClamped.x * sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) * nbProbesPerLayer + probeIndicesClamped.z].colors[1] * distanceScaling;
        case 2: // From left
            return probes[(probeIndicesClamped.x + 1) * sqrt(nbProbesPerLayer) + probeIndicesClamped.y * nbProbesPerLayer + probeIndicesClamped.z].colors[2] * distanceScaling;
        case 3: // From right.
            return probes[(probeIndicesClamped.x - 1) * sqrt(nbProbesPerLayer) + probeIndicesClamped.y * nbProbesPerLayer + probeIndicesClamped.z].colors[3] * distanceScaling;
        case 4: // From front.
            return probes[probeIndicesClamped.x * sqrt(nbProbesPerLayer) + probeIndicesClamped.y * nbProbesPerLayer + (probeIndicesClamped.z + 1)].colors[4] * distanceScaling;
        default: // From back.
            return probes[probeIndicesClamped.x * sqrt(nbProbesPerLayer) + probeIndicesClamped.y * nbProbesPerLayer + (probeIndicesClamped.z - 1)].colors[5] * distanceScaling;
    }
}