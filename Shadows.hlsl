#include "UtilityFunctions.hlsl"

#ifndef SHADOWS
#define SHADOWS

float TraceShadowRayInline(float3 hitWorldPos, float3 lightDir, float length, RaytracingAccelerationStructure SceneBVH, 
    uint frameCount, uint2 randomSeed1, uint2 randomSeed2)
{
    int nbSamples = 1;
    uint randSeed = initRand(randomSeed1.x + randomSeed1.y * randomSeed2.x, frameCount, 16);
    float randFactor = 0;

    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = hitWorldPos;
    ray.TMin = 0.001;
    ray.TMax = length;

    float shadowFactor = 0;
    for (int i = 0; i < nbSamples; i++)
    {
        ray.Direction = float3(lightDir.x + nextRand(randSeed) * randFactor, 
                               lightDir.y + nextRand(randSeed) * randFactor,
                               lightDir.z + nextRand(randSeed) * randFactor);

        q.TraceRayInline(SceneBVH, RAY_FLAG_NONE, 0x01 /*cubes only*/, ray);
        q.Proceed();

        if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            shadowFactor += 1 / (float)nbSamples;
    }

    return 1 - shadowFactor;
}

float CalculateShadows(float3 normal, float3 hitWorldPos, float3 lightDir, float length, RaytracingAccelerationStructure SceneBVH,
                       uint frameCount, uint2 randomSeed1, uint2 randomSeed2)
{
    bool isShadowed = dot(normalize(normal), normalize(lightDir)) < 0.f;

    float shadowFactor = 0;
    // Trace shadow ray if we are not already in shadow.
    if (!isShadowed)
        shadowFactor = TraceShadowRayInline(hitWorldPos, lightDir, length, SceneBVH, frameCount, randomSeed1, randomSeed2);

    return shadowFactor;
}

#endif