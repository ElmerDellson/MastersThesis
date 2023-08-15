#include "Common.hlsl"

#ifndef PLACE_SURFELS
#define PLACE_SURFELS

Surfel PlaceSurfel(float3 probePos, float distBetweenProbes, uint i, uint j, uint nbSurfelsPerProbe,
                   RaytracingAccelerationStructure SceneBVH, uint nbCubesInScene,
                   StructuredBuffer<STriVertex> CubeBTriVertex, StructuredBuffer<int> CubeIndices,
                   StructuredBuffer<STriVertex> IndexedPlaneBTriVertex, StructuredBuffer<int> IndexedPlaneIndices,
                   StructuredBuffer<InstanceProperties> instanceProps)
{
    float sectionsInXZPlane = (float)nbSurfelsPerProbe / 8.0;
    float sectionWidth = (2.0 * pi / sectionsInXZPlane);
    float adjustment = ((float)nbSurfelsPerProbe - 32.0) / 64.0;
    float phi = pi * ((float)j / 8.0) + (pi / 16.0);
    float theta = 2.0 * pi * ((float)i / sectionsInXZPlane) - (sectionWidth * adjustment);

    float3 direction = float3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
    float surfelDistLimit = FindSurfelDistLimit(direction, distBetweenProbes);

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    RayDesc ray;
    ray.Origin = probePos;
    ray.TMin = 0.001;
    ray.TMax = surfelDistLimit;
    ray.Direction = direction;
    q.TraceRayInline(SceneBVH, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, 0x01 | 0x02, ray);
    q.Proceed();

    Surfel surfel = { float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0) };
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
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

        surfel.albedo = hitColor;
        surfel.position = hitWorldPos;
        surfel.normal = normal;
    }
    else
        surfel.normal = float3(0, 0, 0);

    return surfel;
}

void PlaceSurfels(float3 probePos, float distBetweenProbes, uint probeIndex, uint nbSurfelsPerProbe,
                  RWStructuredBuffer<Surfel> surfels, RaytracingAccelerationStructure SceneBVH, uint nbCubesInScene,
                  StructuredBuffer<STriVertex> CubeBTriVertex, StructuredBuffer<int> CubeIndices,
                  StructuredBuffer<STriVertex> IndexedPlaneBTriVertex, StructuredBuffer<int> IndexedPlaneIndices,
                  StructuredBuffer<InstanceProperties> instanceProps) 
{
    uint sectionsInXZPlane = nbSurfelsPerProbe / 8;

    // Upper half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfels[probeIndex * nbSurfelsPerProbe + i * 4 + j] = PlaceSurfel(probePos, distBetweenProbes, i, j, 
                nbSurfelsPerProbe, SceneBVH, nbCubesInScene, CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex, 
                IndexedPlaneIndices, instanceProps);
    }

    // Lower half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfels[probeIndex * nbSurfelsPerProbe + (nbSurfelsPerProbe / 2) + i * 4 + j] =
                PlaceSurfel(probePos, distBetweenProbes, i, j + 4, nbSurfelsPerProbe, SceneBVH, nbCubesInScene, 
                    CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex, IndexedPlaneIndices, instanceProps);
    }
}

SurfelBatch64 PlaceSurfelsAndReturn(float3 probePos, float distBetweenProbes, uint probeIndex, uint nbSurfelsPerProbe,
                                    RaytracingAccelerationStructure SceneBVH, uint nbCubesInScene,
                                    StructuredBuffer<STriVertex> CubeBTriVertex, StructuredBuffer<int> CubeIndices,
                                    StructuredBuffer<STriVertex> IndexedPlaneBTriVertex, StructuredBuffer<int> IndexedPlaneIndices,
                                    StructuredBuffer<InstanceProperties> instanceProps)
{
    SurfelBatch64 surfelBatch;
    uint sectionsInXZPlane = nbSurfelsPerProbe / 8;

    // Upper half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfelBatch.surfels[probeIndex * nbSurfelsPerProbe + i * 4 + j] = PlaceSurfel(probePos, distBetweenProbes, i, j,
                                                                              nbSurfelsPerProbe, SceneBVH, nbCubesInScene, 
                                                                              CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex,
                                                                              IndexedPlaneIndices, instanceProps);
    }

    // Lower half of probe.
    for (uint i = 0; i < sectionsInXZPlane; i++)
    {
        for (uint j = 0; j < 4; j++)
            surfelBatch.surfels[probeIndex * nbSurfelsPerProbe + (nbSurfelsPerProbe / 2) + i * 4 + j] =
                PlaceSurfel(probePos, distBetweenProbes, i, j + 4, nbSurfelsPerProbe, SceneBVH, nbCubesInScene,
                            CubeBTriVertex, CubeIndices, IndexedPlaneBTriVertex, IndexedPlaneIndices, instanceProps);
    }

    return surfelBatch;
}

#endif